#include "Engine/Physics/ConstraintSolver/ConstraintSolver.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

// ── Helper ────────────────────────────────────────────────────────────────────

static float Dot3(const float a[3],const float b[3]){ return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
static void  Sub3(const float a[3],const float b[3],float out[3]){ for(int i=0;i<3;++i)out[i]=a[i]-b[i]; }
static float Len3(const float v[3]){ return std::sqrt(Dot3(v,v)); }
static void  Normalize3(float v[3]){ float l=Len3(v); if(l>1e-7f){v[0]/=l;v[1]/=l;v[2]/=l;} }

struct ConstraintSolver::Impl {
    std::unordered_map<uint32_t, ConstraintEntry>   constraints;
    std::unordered_map<uint32_t, RigidBodyState>    bodies;
    std::unordered_map<uint32_t, RigidBodyState>    corrected;
    uint32_t nextId{1};
    uint32_t iterations{10};

    void SolveDistance(ConstraintEntry& ce, float dt) {
        auto& d=ce.desc;
        auto itA=corrected.find(d.bodyA);
        if(itA==corrected.end()) return;
        auto& bA=itA->second;

        float posB[3]{d.anchorB[0],d.anchorB[1],d.anchorB[2]};
        float invMassB=0.f;
        RigidBodyState* pB=nullptr;
        if(d.bodyB) {
            auto itB=corrected.find(d.bodyB);
            if(itB!=corrected.end()){ pB=&itB->second; invMassB=pB->inverseMass; for(int i=0;i<3;++i)posB[i]=pB->position[i]+d.anchorB[i]; }
        } else {
            for(int i=0;i<3;++i) posB[i]=d.anchorB[i];
        }

        float delta[3]; Sub3(bA.position,posB,delta);
        float dist=Len3(delta);
        if(dist<1e-7f) return;
        float err=dist-d.restLength;
        float n[3]={delta[0]/dist,delta[1]/dist,delta[2]/dist};
        float invMassTotal=bA.inverseMass+invMassB;
        if(invMassTotal<1e-9f) return;
        float lambda=-err/invMassTotal;
        // Warm-start would accumulate ce.lambda here; we do simple single correction
        for(int i=0;i<3;++i) bA.position[i]+=n[i]*lambda*bA.inverseMass;
        if(pB) for(int i=0;i<3;++i) pB->position[i]-=n[i]*lambda*invMassB;
    }

    void SolveSpring(ConstraintEntry& ce, float dt) {
        auto& d=ce.desc;
        auto itA=corrected.find(d.bodyA); if(itA==corrected.end()) return;
        auto& bA=itA->second;
        float anchor[3]={d.anchorB[0],d.anchorB[1],d.anchorB[2]};
        float delta[3]; Sub3(bA.position,anchor,delta);
        float dist=Len3(delta); if(dist<1e-7f) return;
        float n[3]={delta[0]/dist,delta[1]/dist,delta[2]/dist};
        float err=dist-d.restLength;
        float springF=-(d.stiffness*err + d.damping*Dot3(bA.velocity,n));
        float impulse=springF*dt;
        for(int i=0;i<3;++i) bA.velocity[i]+=n[i]*impulse*bA.inverseMass;
    }
};

ConstraintSolver::ConstraintSolver() : m_impl(new Impl()) {}
ConstraintSolver::~ConstraintSolver() { delete m_impl; }
void ConstraintSolver::Init(uint32_t iters){ m_impl->iterations=iters; }
void ConstraintSolver::Shutdown(){ Clear(); }

uint32_t ConstraintSolver::AddConstraint(const ConstraintDesc& desc) {
    uint32_t id=m_impl->nextId++;
    ConstraintEntry& e=m_impl->constraints[id]; e.id=id; e.desc=desc;
    return id;
}
void ConstraintSolver::RemoveConstraint(uint32_t id){ m_impl->constraints.erase(id); }
void ConstraintSolver::EnableConstraint(uint32_t id, bool e){
    auto it=m_impl->constraints.find(id); if(it!=m_impl->constraints.end()) it->second.enabled=e;
}
ConstraintEntry ConstraintSolver::GetConstraint(uint32_t id) const {
    auto it=m_impl->constraints.find(id); return it!=m_impl->constraints.end()?it->second:ConstraintEntry{};
}
std::vector<ConstraintEntry> ConstraintSolver::AllConstraints() const {
    std::vector<ConstraintEntry> out; for(auto&[k,v]:m_impl->constraints) out.push_back(v); return out;
}

void ConstraintSolver::SetBodyState(const RigidBodyState& s){ m_impl->bodies[s.id]=s; }
RigidBodyState ConstraintSolver::GetBodyState(uint32_t id) const {
    auto it=m_impl->bodies.find(id); return it!=m_impl->bodies.end()?it->second:RigidBodyState{};
}
void ConstraintSolver::RemoveBody(uint32_t id){ m_impl->bodies.erase(id); m_impl->corrected.erase(id); }

void ConstraintSolver::Solve(float dt) {
    // Copy bodies to working set
    m_impl->corrected=m_impl->bodies;
    for(uint32_t iter=0;iter<m_impl->iterations;++iter){
        for(auto& [id,ce]: m_impl->constraints) {
            if(!ce.enabled) continue;
            switch(ce.desc.type){
                case ConstraintType::Distance:   m_impl->SolveDistance(ce,dt); break;
                case ConstraintType::Spring:     m_impl->SolveSpring(ce,dt);   break;
                default: m_impl->SolveDistance(ce,dt); break;
            }
        }
    }
}

RigidBodyState ConstraintSolver::GetCorrectedState(uint32_t id) const {
    auto it=m_impl->corrected.find(id); return it!=m_impl->corrected.end()?it->second:GetBodyState(id);
}
void ConstraintSolver::SetIterations(uint32_t n){ m_impl->iterations=n>0?n:1; }
uint32_t ConstraintSolver::GetIterations() const{ return m_impl->iterations; }
void ConstraintSolver::Clear(){ m_impl->constraints.clear(); m_impl->bodies.clear(); m_impl->corrected.clear(); }

} // namespace Engine
