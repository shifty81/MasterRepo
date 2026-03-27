#include "Engine/Animation/IK/IKSystem/IKSystem.h"
#include "Engine/Animation/Skeleton/SkeletonSystem/SkeletonSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

struct IKChain {
    uint32_t      id{0};
    IKChainDesc   desc;
    IKGoal        goal;
    bool          enabled{true};
};

struct IKSystem::Impl {
    std::vector<IKChain> chains;
    uint32_t nextId{1};
    IKChain* Find(uint32_t id) {
        for (auto& c:chains) if(c.id==id) return &c; return nullptr;
    }
    const IKChain* Find(uint32_t id) const {
        for (auto& c:chains) if(c.id==id) return &c; return nullptr;
    }
};

static float VLen(const float v[3]) { return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]); }
static void  VNorm(float v[3]) { float l=VLen(v)+1e-9f; v[0]/=l;v[1]/=l;v[2]/=l; }
static float VDist(const float a[3], const float b[3]) {
    float d[3]={a[0]-b[0],a[1]-b[1],a[2]-b[2]}; return VLen(d);
}

static void SolveTwoBone(const IKChainDesc& desc, const IKGoal& goal,
                          std::vector<JointPose>& pose)
{
    if (desc.jointIndices.size()<3) return;
    uint32_t root=desc.jointIndices[0], mid=desc.jointIndices[1], tip=desc.jointIndices[2];
    if (root>=pose.size()||mid>=pose.size()||tip>=pose.size()) return;

    float* rp=pose[root].translation, *mp=pose[mid].translation, *tp=pose[tip].translation;
    float upperLen = VDist(mp, rp);
    float lowerLen = VDist(tp, mp);
    float targetDist= VDist(goal.position, rp);

    // Clamp to max reach
    float maxReach = upperLen+lowerLen;
    float clampedDist = std::min(targetDist, maxReach*0.9999f);

    // Law of cosines
    float cosAngle = (upperLen*upperLen + clampedDist*clampedDist - lowerLen*lowerLen)
                     / (2.f*upperLen*clampedDist+1e-9f);
    cosAngle = std::max(-1.f, std::min(1.f, cosAngle));
    float angle = std::acos(cosAngle);

    // Direction from root to target
    float dir[3];
    for (int i=0;i<3;i++) dir[i]=goal.position[i]-rp[i];
    VNorm(dir);

    // Place mid joint along dir rotated by angle
    float midPos[3];
    for (int i=0;i<3;i++) midPos[i] = rp[i] + dir[i]*upperLen*std::cos(angle);
    // Perpendicular component (use pole vector)
    float perp[3] = {desc.poleVector[0],desc.poleVector[1],desc.poleVector[2]};
    VNorm(perp);
    float sinAngle = std::sin(angle);
    for (int i=0;i<3;i++) midPos[i] += perp[i]*upperLen*sinAngle;

    // Blend with original pose
    float w = desc.blendWeight;
    for (int i=0;i<3;i++) pose[mid].translation[i] = pose[mid].translation[i]*(1.f-w) + midPos[i]*w;
    for (int i=0;i<3;i++) pose[tip].translation[i] = pose[tip].translation[i]*(1.f-w) + goal.position[i]*w;
}

static void SolveFABRIK(const IKChainDesc& desc, const IKGoal& goal,
                         std::vector<JointPose>& pose)
{
    const auto& ji = desc.jointIndices;
    uint32_t n = (uint32_t)ji.size();
    if (n < 2) return;

    // Store positions
    std::vector<float[3]> pos(n);
    for (uint32_t i=0;i<n;i++) {
        if (ji[i]<pose.size())
            for (int k=0;k<3;k++) pos[i][k]=pose[ji[i]].translation[k];
    }
    // Bone lengths
    std::vector<float> lens(n-1);
    for (uint32_t i=0;i<n-1;i++) lens[i]=VDist(pos[i+1],pos[i]);

    float totalLen=0.f; for(auto l:lens) totalLen+=l;
    float targetDist=VDist(goal.position, pos[0]);
    if (targetDist > totalLen) {
        // Stretch toward target
        float dir[3]; for(int k=0;k<3;k++) dir[k]=goal.position[k]-pos[0][k]; VNorm(dir);
        float acc=0.f;
        for(uint32_t i=1;i<n;i++) {
            acc+=lens[i-1];
            for(int k=0;k<3;k++) pos[i][k]=pos[0][k]+dir[k]*acc;
        }
    } else {
        // FABRIK iterations
        float root[3]; for(int k=0;k<3;k++) root[k]=pos[0][k];
        for (uint32_t iter=0; iter<desc.maxIterations; iter++) {
            // Forward
            for(int k=0;k<3;k++) pos[n-1][k]=goal.position[k];
            for (int i=(int)n-2;i>=0;i--) {
                float d[3]; for(int k=0;k<3;k++) d[k]=pos[i][k]-pos[i+1][k]; VNorm(d);
                for(int k=0;k<3;k++) pos[i][k]=pos[i+1][k]+d[k]*lens[i];
            }
            // Backward
            for(int k=0;k<3;k++) pos[0][k]=root[k];
            for (uint32_t i=0;i<n-1;i++) {
                float d[3]; for(int k=0;k<3;k++) d[k]=pos[i+1][k]-pos[i][k]; VNorm(d);
                for(int k=0;k<3;k++) pos[i+1][k]=pos[i][k]+d[k]*lens[i];
            }
            if (VDist(pos[n-1],goal.position)<desc.tolerance) break;
        }
    }
    // Write back with blend
    float w=desc.blendWeight;
    for (uint32_t i=0;i<n;i++) if(ji[i]<pose.size())
        for(int k=0;k<3;k++) pose[ji[i]].translation[k]=
            pose[ji[i]].translation[k]*(1.f-w)+pos[i][k]*w;
}

IKSystem::IKSystem()  : m_impl(new Impl) {}
IKSystem::~IKSystem() { Shutdown(); delete m_impl; }

void IKSystem::Init()     {}
void IKSystem::Shutdown() { m_impl->chains.clear(); }

uint32_t IKSystem::AddChain(const IKChainDesc& desc) {
    IKChain c; c.id=m_impl->nextId++; c.desc=desc;
    m_impl->chains.push_back(c); return m_impl->chains.back().id;
}
void IKSystem::RemoveChain(uint32_t id) {
    auto& v=m_impl->chains;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& c){return c.id==id;}),v.end());
}
bool IKSystem::HasChain(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void IKSystem::SetGoal(uint32_t id, const float pos[3], const float ori[4]) {
    auto* c=m_impl->Find(id); if(!c) return;
    for(int i=0;i<3;i++) c->goal.position[i]=pos[i];
    if (ori) { for(int i=0;i<4;i++) c->goal.orientation[i]=ori[i]; c->goal.useOrientation=true; }
}
void IKSystem::SetPoleVector(uint32_t id, const float pole[3]) {
    auto* c=m_impl->Find(id); if(c) for(int i=0;i<3;i++) c->desc.poleVector[i]=pole[i];
}
void IKSystem::SetBlendWeight(uint32_t id, float w) {
    auto* c=m_impl->Find(id); if(c) c->desc.blendWeight=w;
}
void IKSystem::SetEnabled(uint32_t id, bool e) {
    auto* c=m_impl->Find(id); if(c) c->enabled=e;
}

void IKSystem::Solve(uint32_t id, std::vector<JointPose>& pose) const {
    const auto* c=m_impl->Find(id); if(!c||!c->enabled) return;
    if (c->desc.solver==IKSolverType::TwoBone)
        SolveTwoBone(c->desc, c->goal, pose);
    else
        SolveFABRIK(c->desc, c->goal, pose);
}

void IKSystem::SolveAll(std::vector<JointPose>& pose) const {
    for (auto& c : m_impl->chains) if(c.enabled) Solve(c.id, pose);
}

float IKSystem::GetBlendWeight(uint32_t id) const {
    const auto* c=m_impl->Find(id); return c?c->desc.blendWeight:0.f;
}
bool IKSystem::IsEnabled(uint32_t id) const {
    const auto* c=m_impl->Find(id); return c&&c->enabled;
}
bool IKSystem::InReach(uint32_t id, const float target[3]) const {
    const auto* c=m_impl->Find(id); if(!c||c->desc.jointIndices.empty()) return false;
    (void)target; return true; // simplified
}
uint32_t IKSystem::ChainCount() const { return (uint32_t)m_impl->chains.size(); }

} // namespace Engine
