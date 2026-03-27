#include "Engine/Physics/Ragdoll/RagdollSystem/RagdollSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

struct RigidBody {
    float pos[3]{}, vel[3]{}, angVel[3]{};
    float orientation[4]{0,0,0,1};
    float invMass{1.f};
    float force[3]{};
    bool  sleeping{false};
};

struct RagdollData {
    uint32_t           id{0};
    RagdollDesc        desc;
    std::vector<RigidBody> bodies;
    std::vector<BoneTransform> animPose;
    float              blendWeight{0.f};
    bool               dead{false};
    bool               sleeping{false};
};

struct RagdollSystem::Impl {
    std::vector<RagdollData> ragdolls;
    uint32_t nextId{1};
    std::function<void(uint32_t)> onSleep;

    RagdollData* Find(uint32_t id){
        for(auto& r:ragdolls) if(r.id==id) return &r; return nullptr;
    }
    const RagdollData* Find(uint32_t id) const {
        for(auto& r:ragdolls) if(r.id==id) return &r; return nullptr;
    }
};

RagdollSystem::RagdollSystem()  : m_impl(new Impl){}
RagdollSystem::~RagdollSystem() { Shutdown(); delete m_impl; }
void RagdollSystem::Init()     {}
void RagdollSystem::Shutdown() { m_impl->ragdolls.clear(); }

uint32_t RagdollSystem::CreateRagdoll(const RagdollDesc& desc)
{
    RagdollData rd; rd.id=m_impl->nextId++; rd.desc=desc;
    for(uint32_t i=0;i<desc.boneCount;i++){
        RigidBody rb;
        for(int j=0;j<3;j++) rb.pos[j]=desc.bones[i].localPos[j];
        rb.invMass=1.f/std::max(0.01f,desc.bones[i].mass);
        rd.bodies.push_back(rb);
        rd.animPose.push_back({});
    }
    m_impl->ragdolls.push_back(rd);
    return m_impl->ragdolls.back().id;
}

void RagdollSystem::DestroyRagdoll(uint32_t id){
    auto& v=m_impl->ragdolls;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& r){return r.id==id;}),v.end());
}
bool RagdollSystem::HasRagdoll(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void RagdollSystem::SetPose(uint32_t id, const BoneTransform* pose){
    auto* r=m_impl->Find(id); if(!r) return;
    for(uint32_t i=0;i<r->desc.boneCount;i++) r->animPose[i]=pose[i];
}
void RagdollSystem::SetBlendWeight(uint32_t id, float w){
    auto* r=m_impl->Find(id); if(r) r->blendWeight=std::max(0.f,std::min(1.f,w));
}
float RagdollSystem::GetBlendWeight(uint32_t id) const {
    const auto* r=m_impl->Find(id); return r?r->blendWeight:0.f;
}
void RagdollSystem::SetDead(uint32_t id){
    auto* r=m_impl->Find(id); if(r){ r->dead=true; r->blendWeight=1.f; }
}
bool RagdollSystem::IsDead    (uint32_t id) const { const auto* r=m_impl->Find(id); return r&&r->dead; }
bool RagdollSystem::IsSleeping(uint32_t id) const { const auto* r=m_impl->Find(id); return r&&r->sleeping; }

void RagdollSystem::AddImpulse(uint32_t id, uint32_t bi, const float imp[3]){
    auto* r=m_impl->Find(id);
    if(r&&bi<r->bodies.size())
        for(int i=0;i<3;i++) r->bodies[bi].vel[i]+=imp[i]*r->bodies[bi].invMass;
}

BoneTransform RagdollSystem::GetBoneTransform(uint32_t id, uint32_t bi) const {
    const auto* r=m_impl->Find(id);
    if(!r||bi>=r->bodies.size()) return {};
    BoneTransform bt;
    if(r->blendWeight<1.f){
        float w=r->blendWeight;
        for(int i=0;i<3;i++)
            bt.position[i]=(1.f-w)*r->animPose[bi].position[i]+w*r->bodies[bi].pos[i];
        for(int i=0;i<4;i++) bt.orientation[i]=r->animPose[bi].orientation[i];
    } else {
        for(int i=0;i<3;i++) bt.position[i]=r->bodies[bi].pos[i];
        for(int i=0;i<4;i++) bt.orientation[i]=r->bodies[bi].orientation[i];
    }
    return bt;
}
uint32_t RagdollSystem::BoneCount(uint32_t id) const {
    const auto* r=m_impl->Find(id); return r?r->desc.boneCount:0;
}

void RagdollSystem::SetOnSleep(std::function<void(uint32_t)> cb){ m_impl->onSleep=cb; }
std::vector<uint32_t> RagdollSystem::GetAll() const {
    std::vector<uint32_t> out; for(auto& r:m_impl->ragdolls) out.push_back(r.id); return out;
}

void RagdollSystem::Tick(float dt)
{
    for(auto& rd : m_impl->ragdolls){
        if(!rd.dead) continue;
        if(rd.sleeping) continue;
        bool allSleep=true;
        for(uint32_t i=0;i<rd.desc.boneCount;i++){
            auto& b=rd.bodies[i];
            // Gravity
            for(int j=0;j<3;j++) b.force[j]=rd.desc.gravity[j]/b.invMass;
            // Integrate velocity
            for(int j=0;j<3;j++) b.vel[j]+=b.force[j]*b.invMass*dt;
            // Integrate position
            for(int j=0;j<3;j++) b.pos[j]+=b.vel[j]*dt;
            // Damping
            for(int j=0;j<3;j++){b.vel[j]*=0.98f; b.force[j]=0.f;}
            // Floor
            if(b.pos[1]<0.f){b.pos[1]=0.f; b.vel[1]=std::abs(b.vel[1])*0.3f;}
            // Sleep check
            float spd=std::sqrt(b.vel[0]*b.vel[0]+b.vel[1]*b.vel[1]+b.vel[2]*b.vel[2]);
            if(spd>rd.desc.sleepThreshold) allSleep=false;
        }
        if(allSleep&&!rd.sleeping){
            rd.sleeping=true;
            if(m_impl->onSleep) m_impl->onSleep(rd.id);
        }
    }
}

} // namespace Engine
