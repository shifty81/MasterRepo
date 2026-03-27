#include "Engine/Animation/Procedural/ProceduralAnimation/ProceduralAnimation.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

static inline float Len3(PAVec3 v){ return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); }
static inline PAVec3 Norm3(PAVec3 v){ float l=Len3(v); return l>0?PAVec3{v.x/l,v.y/l,v.z/l}:PAVec3{0,1,0}; }
static inline PAVec3 Add3(PAVec3 a,PAVec3 b){ return {a.x+b.x,a.y+b.y,a.z+b.z}; }
static inline PAVec3 Sub3(PAVec3 a,PAVec3 b){ return {a.x-b.x,a.y-b.y,a.z-b.z}; }
static inline PAVec3 Mul3(PAVec3 a,float s){ return {a.x*s,a.y*s,a.z*s}; }
static inline float  Dot3(PAVec3 a,PAVec3 b){ return a.x*b.x+a.y*b.y+a.z*b.z; }

struct BoneChain { uint32_t id; std::vector<uint32_t> boneIds; };

struct SecondaryBoneConfig { uint32_t boneId; float stiffness,damping,inertia; };

struct AgentBoneState {
    std::unordered_map<uint32_t,PAVec3> bonePos;
    std::unordered_map<uint32_t,PAVec3> boneVel;   // secondary motion
    // IK targets per chain
    std::unordered_map<uint32_t,std::pair<PAVec3,float>> ikTargets;
    // Look-at
    uint32_t lookAtBone{0}; PAVec3 lookAtTarget{}; float lookAtWeight{0};
    // Foot placement
    bool     footPlacementOn{false};
    PAVec3   adjustedLeft{}, adjustedRight{};
};

struct ProceduralAnimation::Impl {
    std::vector<BoneChain>           chains;
    std::vector<SecondaryBoneConfig> secondaryBones;
    std::unordered_map<uint32_t,AgentBoneState> agents;

    BoneChain* FindChain(uint32_t id){
        for(auto& c:chains) if(c.id==id) return &c; return nullptr;
    }
    AgentBoneState& GetAgent(uint32_t id){ return agents[id]; }
};

ProceduralAnimation::ProceduralAnimation(): m_impl(new Impl){}
ProceduralAnimation::~ProceduralAnimation(){ Shutdown(); delete m_impl; }
void ProceduralAnimation::Init(){}
void ProceduralAnimation::Shutdown(){ m_impl->chains.clear(); m_impl->agents.clear(); }

void ProceduralAnimation::RegisterBoneChain(uint32_t cid, const uint32_t* boneIds, uint32_t count){
    BoneChain c; c.id=cid;
    c.boneIds.assign(boneIds,boneIds+count);
    m_impl->chains.push_back(c);
}
void ProceduralAnimation::SetIKTarget(uint32_t agentId, uint32_t chainId, PAVec3 target, float weight){
    m_impl->GetAgent(agentId).ikTargets[chainId]={target,weight};
}

void ProceduralAnimation::SolveIK(uint32_t agentId, uint32_t chainId,
                                    uint32_t iterations, std::vector<PAVec3>& outJoints){
    auto* chain=m_impl->FindChain(chainId); if(!chain) return;
    auto& ag=m_impl->GetAgent(agentId);
    uint32_t n=(uint32_t)chain->boneIds.size();
    if(n<2) return;

    // Gather current joint positions
    outJoints.resize(n);
    for(uint32_t i=0;i<n;i++){
        auto it=ag.bonePos.find(chain->boneIds[i]);
        outJoints[i]=it!=ag.bonePos.end()?it->second:PAVec3{0,(float)i,0};
    }

    // FABRIK
    PAVec3 target=outJoints.back();
    {
        auto it2=ag.ikTargets.find(chainId);
        if(it2!=ag.ikTargets.end()) target=it2->second.first;
    }
    // Compute bone lengths
    std::vector<float> lens(n-1);
    for(uint32_t i=0;i+1<n;i++) lens[i]=Len3(Sub3(outJoints[i+1],outJoints[i]));

    PAVec3 root=outJoints[0];
    for(uint32_t iter=0;iter<iterations;iter++){
        // Forward pass
        outJoints.back()=target;
        for(int32_t i=(int32_t)n-2;i>=0;i--){
            PAVec3 dir=Norm3(Sub3(outJoints[i],outJoints[i+1]));
            outJoints[i]=Add3(outJoints[i+1],Mul3(dir,lens[i]));
        }
        // Backward pass
        outJoints[0]=root;
        for(uint32_t i=0;i+1<n;i++){
            PAVec3 dir=Norm3(Sub3(outJoints[i+1],outJoints[i]));
            outJoints[i+1]=Add3(outJoints[i],Mul3(dir,lens[i]));
        }
    }
    // Write back
    for(uint32_t i=0;i<n;i++) ag.bonePos[chain->boneIds[i]]=outJoints[i];
}

void ProceduralAnimation::SetLookAtTarget(uint32_t agentId, uint32_t headBoneId,
                                           PAVec3 target, float weight){
    auto& ag=m_impl->GetAgent(agentId);
    ag.lookAtBone=headBoneId; ag.lookAtTarget=target; ag.lookAtWeight=weight;
}
PAQuat ProceduralAnimation::SolveLookAt(uint32_t agentId){
    auto& ag=m_impl->GetAgent(agentId);
    auto it=ag.bonePos.find(ag.lookAtBone);
    PAVec3 origin=it!=ag.bonePos.end()?it->second:PAVec3{0,0,0};
    PAVec3 dir=Norm3(Sub3(ag.lookAtTarget,origin));
    // Quaternion from forward (0,0,1) to dir
    PAVec3 fwd={0,0,1};
    float d=Dot3(fwd,dir);
    PAQuat q;
    if(d<-0.9999f){ q={0,1,0,0}; }
    else {
        PAVec3 ax={fwd.y*dir.z-fwd.z*dir.y, fwd.z*dir.x-fwd.x*dir.z, fwd.x*dir.y-fwd.y*dir.x};
        q={ax.x,ax.y,ax.z,1+d};
        float l=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);
        if(l>0){ q.x/=l; q.y/=l; q.z/=l; q.w/=l; }
    }
    // Blend with identity by weight
    float w=ag.lookAtWeight;
    q.x*=w; q.y*=w; q.z*=w; q.w=1-(1-q.w)*w;
    return q;
}

void ProceduralAnimation::SetFootPlacementEnabled(uint32_t agentId, bool on){
    m_impl->GetAgent(agentId).footPlacementOn=on;
}
void ProceduralAnimation::UpdateFootPlacement(uint32_t agentId,
    PAVec3 leftFoot, PAVec3 rightFoot,
    std::function<float(PAVec3,PAVec3)> groundQuery){
    auto& ag=m_impl->GetAgent(agentId);
    if(!ag.footPlacementOn){ ag.adjustedLeft=leftFoot; ag.adjustedRight=rightFoot; return; }
    PAVec3 dn={0,-1,0};
    float lh=groundQuery(leftFoot,dn);
    float rh=groundQuery(rightFoot,dn);
    ag.adjustedLeft ={leftFoot.x, leftFoot.y-lh, leftFoot.z};
    ag.adjustedRight={rightFoot.x,rightFoot.y-rh,rightFoot.z};
}
PAVec3 ProceduralAnimation::GetAdjustedFootPos(uint32_t agentId, bool leftFoot) const {
    auto it=m_impl->agents.find(agentId);
    if(it==m_impl->agents.end()) return {};
    return leftFoot?it->second.adjustedLeft:it->second.adjustedRight;
}

void ProceduralAnimation::AddSecondaryBone(uint32_t boneId, float stiff, float damp, float inert){
    SecondaryBoneConfig c; c.boneId=boneId; c.stiffness=stiff; c.damping=damp; c.inertia=inert;
    m_impl->secondaryBones.push_back(c);
}
void ProceduralAnimation::TickSecondary(uint32_t agentId, float dt){
    auto& ag=m_impl->GetAgent(agentId);
    for(auto& cfg:m_impl->secondaryBones){
        auto& vel=ag.boneVel[cfg.boneId];
        auto& pos=ag.bonePos[cfg.boneId];
        // Spring toward zero offset
        PAVec3 force={-cfg.stiffness*pos.x-cfg.damping*vel.x,
                      -cfg.stiffness*pos.y-cfg.damping*vel.y,
                      -cfg.stiffness*pos.z-cfg.damping*vel.z};
        vel.x+=force.x*dt; vel.y+=force.y*dt; vel.z+=force.z*dt;
        pos.x+=vel.x*dt;   pos.y+=vel.y*dt;   pos.z+=vel.z*dt;
    }
}
PAVec3 ProceduralAnimation::GetSecondaryOffset(uint32_t agentId, uint32_t boneId) const {
    auto ait=m_impl->agents.find(agentId); if(ait==m_impl->agents.end()) return {};
    auto bit=ait->second.bonePos.find(boneId);
    return bit!=ait->second.bonePos.end()?bit->second:PAVec3{};
}

void   ProceduralAnimation::SetBoneWorldPos(uint32_t agentId, uint32_t boneId, PAVec3 pos){
    m_impl->GetAgent(agentId).bonePos[boneId]=pos;
}
PAVec3 ProceduralAnimation::GetBoneWorldPos(uint32_t agentId, uint32_t boneId) const {
    auto ait=m_impl->agents.find(agentId); if(ait==m_impl->agents.end()) return {};
    auto bit=ait->second.bonePos.find(boneId);
    return bit!=ait->second.bonePos.end()?bit->second:PAVec3{};
}
void ProceduralAnimation::Reset(uint32_t agentId){ m_impl->agents.erase(agentId); }

} // namespace Engine
