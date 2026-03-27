#include "Engine/Animation/Skeleton/SkeletonSystem/SkeletonSystem.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Engine {

// ── Quaternion helpers ───────────────────────────────────────────────────────

static void QuatMul(const float a[4], const float b[4], float out[4]) {
    out[0]=a[3]*b[0]+a[0]*b[3]+a[1]*b[2]-a[2]*b[1];
    out[1]=a[3]*b[1]-a[0]*b[2]+a[1]*b[3]+a[2]*b[0];
    out[2]=a[3]*b[2]+a[0]*b[1]-a[1]*b[0]+a[2]*b[3];
    out[3]=a[3]*b[3]-a[0]*b[0]-a[1]*b[1]-a[2]*b[2];
}

static void QuatNorm(float q[4]) {
    float len=std::sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3]);
    if (len>1e-6f){q[0]/=len;q[1]/=len;q[2]/=len;q[3]/=len;}
}

// Compose local pose into 4x4 column-major matrix
static void PoseToMatrix(const JointPose& p, float m[16]) {
    float q[4]={p.rotation[0],p.rotation[1],p.rotation[2],p.rotation[3]};
    QuatNorm(q);
    float qx=q[0],qy=q[1],qz=q[2],qw=q[3];
    float sx=p.scale[0], sy=p.scale[1], sz=p.scale[2];
    m[ 0]=(1-2*(qy*qy+qz*qz))*sx; m[ 4]=(2*(qx*qy-qz*qw))*sy;   m[ 8]=(2*(qx*qz+qy*qw))*sz;   m[12]=p.translation[0];
    m[ 1]=(2*(qx*qy+qz*qw))*sx;   m[ 5]=(1-2*(qx*qx+qz*qz))*sy; m[ 9]=(2*(qy*qz-qx*qw))*sz;   m[13]=p.translation[1];
    m[ 2]=(2*(qx*qz-qy*qw))*sx;   m[ 6]=(2*(qy*qz+qx*qw))*sy;   m[10]=(1-2*(qx*qx+qy*qy))*sz; m[14]=p.translation[2];
    m[ 3]=0; m[ 7]=0; m[11]=0; m[15]=1;
}

// Multiply two 4x4 column-major matrices: out = a * b
static void MatMul(const float a[16], const float b[16], float out[16]) {
    for(int r=0;r<4;r++) for(int c=0;c<4;c++) {
        out[c*4+r]=0;
        for(int k=0;k<4;k++) out[c*4+r]+=a[k*4+r]*b[c*4+k];
    }
}

// Invert a 4x4 column-major matrix (assumes TRS, not full inversion)
static void MatInverseTRS(const float m[16], float inv[16]) {
    // Extract scale
    float sx=std::sqrt(m[0]*m[0]+m[1]*m[1]+m[2]*m[2]);
    float sy=std::sqrt(m[4]*m[4]+m[5]*m[5]+m[6]*m[6]);
    float sz=std::sqrt(m[8]*m[8]+m[9]*m[9]+m[10]*m[10]);
    if(sx<1e-6f)sx=1;if(sy<1e-6f)sy=1;if(sz<1e-6f)sz=1;
    float isx=1/sx,isy=1/sy,isz=1/sz;
    // Transpose upper-left 3x3 with scale inverse
    inv[0]=m[0]*isx*isx; inv[4]=m[1]*isy*isy; inv[ 8]=m[2]*isz*isz;
    inv[1]=m[4]*isx*isx; inv[5]=m[5]*isy*isy; inv[ 9]=m[6]*isz*isz;
    inv[2]=m[8]*isx*isx; inv[6]=m[9]*isy*isy; inv[10]=m[10]*isz*isz;
    // Translation: -R^T * t
    float tx=m[12],ty=m[13],tz=m[14];
    inv[12]=-(inv[0]*tx+inv[4]*ty+inv[ 8]*tz);
    inv[13]=-(inv[1]*tx+inv[5]*ty+inv[ 9]*tz);
    inv[14]=-(inv[2]*tx+inv[6]*ty+inv[10]*tz);
    inv[3]=inv[7]=inv[11]=0; inv[15]=1;
}

// ── Impl ─────────────────────────────────────────────────────────────────────

struct SkeletonSystem::Impl {
    std::unordered_map<std::string, SkeletonDef>        defs;
    std::unordered_map<uint32_t, SkeletonInstance>      instances;
    std::unordered_map<std::string, std::vector<float>> bindInverse; ///< defId → flat inverse-bind mats
    uint32_t nextId{1};

    void ComputeBindInverse(const SkeletonDef& def) {
        uint32_t N = (uint32_t)def.joints.size();
        std::vector<float> local(N*16);
        std::vector<float> world(N*16);
        for (uint32_t i=0;i<N;i++) PoseToMatrix(def.joints[i].bindPose, &local[i*16]);
        for (uint32_t i=0;i<N;i++) {
            int32_t p = def.joints[i].parentIndex;
            if (p<0) { for(int k=0;k<16;k++) world[i*16+k]=local[i*16+k]; }
            else       MatMul(&world[p*16], &local[i*16], &world[i*16]);
        }
        std::vector<float> inv(N*16);
        for (uint32_t i=0;i<N;i++) MatInverseTRS(&world[i*16], &inv[i*16]);
        bindInverse[def.id] = inv;
    }
};

SkeletonSystem::SkeletonSystem()  : m_impl(new Impl) {}
SkeletonSystem::~SkeletonSystem() { Shutdown(); delete m_impl; }

void SkeletonSystem::Init()     {}
void SkeletonSystem::Shutdown() { m_impl->instances.clear(); }

void SkeletonSystem::RegisterDefinition(const SkeletonDef& def)
{
    m_impl->defs[def.id] = def;
    m_impl->ComputeBindInverse(def);
}

bool SkeletonSystem::LoadDefinition(const std::string& path)
{
    std::ifstream f(path); return f.good();
}

bool SkeletonSystem::HasDefinition(const std::string& defId) const
{ return m_impl->defs.count(defId)>0; }

const SkeletonDef* SkeletonSystem::GetDefinition(const std::string& defId) const
{
    auto it = m_impl->defs.find(defId);
    return it!=m_impl->defs.end() ? &it->second : nullptr;
}

uint32_t SkeletonSystem::CreateInstance(const std::string& defId)
{
    auto dit = m_impl->defs.find(defId);
    if (dit==m_impl->defs.end()) return 0;
    const auto& def = dit->second;
    uint32_t id = m_impl->nextId++;
    SkeletonInstance inst;
    inst.instanceId = id;
    inst.defId      = defId;
    inst.localPoses = std::vector<JointPose>(def.joints.size());
    for (uint32_t i=0;i<(uint32_t)def.joints.size();i++) inst.localPoses[i]=def.joints[i].bindPose;
    inst.worldMatrices.resize(def.joints.size()*16, 0.f);
    inst.skinMatrices.resize(def.joints.size()*16, 0.f);
    m_impl->instances[id] = std::move(inst);
    return id;
}

void SkeletonSystem::DestroyInstance(uint32_t id) { m_impl->instances.erase(id); }
bool SkeletonSystem::HasInstance    (uint32_t id) const { return m_impl->instances.count(id)>0; }

void SkeletonSystem::SetLocalPose(uint32_t instanceId, uint32_t jointIdx,
                                    const float tr[3], const float rot[4], const float sc[3])
{
    auto it = m_impl->instances.find(instanceId);
    if (it==m_impl->instances.end()) return;
    auto& inst = it->second;
    if (jointIdx>=(uint32_t)inst.localPoses.size()) return;
    auto& p = inst.localPoses[jointIdx];
    if (tr)  { p.translation[0]=tr[0]; p.translation[1]=tr[1]; p.translation[2]=tr[2]; }
    if (rot) { p.rotation[0]=rot[0]; p.rotation[1]=rot[1]; p.rotation[2]=rot[2]; p.rotation[3]=rot[3]; }
    if (sc)  { p.scale[0]=sc[0]; p.scale[1]=sc[1]; p.scale[2]=sc[2]; }
    inst.dirty=true;
}

void SkeletonSystem::SetLocalPose(uint32_t instanceId, uint32_t jointIdx,
                                    const JointPose& pose)
{
    auto it = m_impl->instances.find(instanceId);
    if (it==m_impl->instances.end()) return;
    if (jointIdx<(uint32_t)it->second.localPoses.size()) {
        it->second.localPoses[jointIdx]=pose; it->second.dirty=true;
    }
}

void SkeletonSystem::ResetToBindPose(uint32_t instanceId)
{
    auto it = m_impl->instances.find(instanceId);
    if (it==m_impl->instances.end()) return;
    const auto* def = GetDefinition(it->second.defId);
    if (!def) return;
    for (uint32_t i=0;i<(uint32_t)def->joints.size();i++)
        it->second.localPoses[i]=def->joints[i].bindPose;
    it->second.dirty=true;
}

void SkeletonSystem::UpdateWorldMatrices(uint32_t instanceId)
{
    auto it = m_impl->instances.find(instanceId);
    if (it==m_impl->instances.end()) return;
    auto& inst = it->second;
    const auto* def = GetDefinition(inst.defId);
    if (!def) return;
    uint32_t N=(uint32_t)def->joints.size();
    std::vector<float> local(N*16);
    for (uint32_t i=0;i<N;i++) PoseToMatrix(inst.localPoses[i], &local[i*16]);
    for (uint32_t i=0;i<N;i++) {
        int32_t p=def->joints[i].parentIndex;
        if (p<0) for(int k=0;k<16;k++) inst.worldMatrices[i*16+k]=local[i*16+k];
        else     MatMul(&inst.worldMatrices[p*16], &local[i*16], &inst.worldMatrices[i*16]);
    }
    auto bit = m_impl->bindInverse.find(inst.defId);
    if (bit!=m_impl->bindInverse.end()) {
        for (uint32_t i=0;i<N;i++)
            MatMul(&inst.worldMatrices[i*16], &bit->second[i*16], &inst.skinMatrices[i*16]);
    }
    inst.dirty=false;
}

const float* SkeletonSystem::GetSkinMatrices(uint32_t id) const
{
    auto it = m_impl->instances.find(id);
    return it!=m_impl->instances.end() && !it->second.skinMatrices.empty()
           ? it->second.skinMatrices.data() : nullptr;
}

const float* SkeletonSystem::GetWorldMatrix(uint32_t id, uint32_t joint) const
{
    auto it = m_impl->instances.find(id);
    if (it==m_impl->instances.end()) return nullptr;
    uint32_t off=joint*16;
    return off+(uint32_t)it->second.worldMatrices.size()<=it->second.worldMatrices.size()
           ? &it->second.worldMatrices[off] : nullptr;
}

void SkeletonSystem::GetJointWorldPos(uint32_t id, uint32_t joint, float out[3]) const
{
    const float* m = GetWorldMatrix(id,joint);
    if (m) { out[0]=m[12]; out[1]=m[13]; out[2]=m[14]; }
}

int32_t SkeletonSystem::FindJoint(uint32_t id, const std::string& name) const
{
    auto it = m_impl->instances.find(id);
    if (it==m_impl->instances.end()) return -1;
    const auto* def = GetDefinition(it->second.defId);
    if (!def) return -1;
    for (uint32_t i=0;i<(uint32_t)def->joints.size();i++)
        if (def->joints[i].name==name) return (int32_t)i;
    return -1;
}

uint32_t SkeletonSystem::JointCount(uint32_t id) const
{
    auto it = m_impl->instances.find(id);
    return it!=m_impl->instances.end() ? (uint32_t)it->second.localPoses.size() : 0;
}

void SkeletonSystem::BlendPoses(uint32_t instA, uint32_t instB, uint32_t instOut, float t)
{
    auto ia=m_impl->instances.find(instA), ib=m_impl->instances.find(instB),
         io=m_impl->instances.find(instOut);
    if (ia==m_impl->instances.end()||ib==m_impl->instances.end()||io==m_impl->instances.end()) return;
    uint32_t N=(uint32_t)std::min({ia->second.localPoses.size(),
                                   ib->second.localPoses.size(),
                                   io->second.localPoses.size()});
    for (uint32_t i=0;i<N;i++) {
        const auto& pa=ia->second.localPoses[i];
        const auto& pb=ib->second.localPoses[i];
        auto&       po=io->second.localPoses[i];
        for(int k=0;k<3;k++) po.translation[k]=pa.translation[k]+(pb.translation[k]-pa.translation[k])*t;
        for(int k=0;k<4;k++) po.rotation[k]=pa.rotation[k]+(pb.rotation[k]-pa.rotation[k])*t;
        for(int k=0;k<3;k++) po.scale[k]=pa.scale[k]+(pb.scale[k]-pa.scale[k])*t;
    }
    io->second.dirty=true;
}

} // namespace Engine
