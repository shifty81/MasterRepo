#include "Engine/Physics/Constraints/ConstraintSystem/ConstraintSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct Constraint {
    uint32_t        id;
    ConstraintType  type;
    uint32_t        bodyA, bodyB;
    ConstraintParams params;
    bool            enabled{true};
    float           appliedForce{0};
    float           breakForce;
    std::function<void()> onBreak;
};

struct ConstraintSystem::Impl {
    uint32_t nextId{1};
    uint32_t iterations{10};
    std::vector<Constraint>             constraints;
    std::unordered_map<uint32_t,BodyState> bodies;

    Constraint* Find(uint32_t id){
        for(auto& c:constraints) if(c.id==id) return &c;
        return nullptr;
    }
    const Constraint* Find(uint32_t id) const {
        for(auto& c:constraints) if(c.id==id) return &c;
        return nullptr;
    }
};

ConstraintSystem::ConstraintSystem(): m_impl(new Impl){}
ConstraintSystem::~ConstraintSystem(){ Shutdown(); delete m_impl; }
void ConstraintSystem::Init(){}
void ConstraintSystem::Shutdown(){ m_impl->constraints.clear(); m_impl->bodies.clear(); }
void ConstraintSystem::Reset(){ Shutdown(); m_impl->nextId=1; }

void ConstraintSystem::SetBodyState(uint32_t id, const BodyState& s){ m_impl->bodies[id]=s; }
BodyState ConstraintSystem::GetBodyState(uint32_t id) const {
    auto it=m_impl->bodies.find(id);
    return it!=m_impl->bodies.end()?it->second:BodyState{};
}

uint32_t ConstraintSystem::CreateConstraint(ConstraintType t, uint32_t bA, uint32_t bB,
                                             const ConstraintParams& p){
    Constraint c; c.id=m_impl->nextId++; c.type=t;
    c.bodyA=bA; c.bodyB=bB; c.params=p; c.breakForce=p.breakForce;
    m_impl->constraints.push_back(c); return c.id;
}
void ConstraintSystem::DestroyConstraint(uint32_t id){
    m_impl->constraints.erase(
        std::remove_if(m_impl->constraints.begin(),m_impl->constraints.end(),
            [id](const Constraint& c){return c.id==id;}),
        m_impl->constraints.end());
}
void ConstraintSystem::SetEnabled(uint32_t id, bool en){
    auto* c=m_impl->Find(id); if(c) c->enabled=en;
}
bool ConstraintSystem::IsEnabled(uint32_t id) const {
    auto* c=m_impl->Find(id); return c&&c->enabled;
}
void ConstraintSystem::SetBreakForce(uint32_t id, float f){
    auto* c=m_impl->Find(id); if(c) c->breakForce=f;
}
void ConstraintSystem::SetLimits(uint32_t id, float mn, float mx){
    auto* c=m_impl->Find(id);
    if(c){ c->params.minLimit=mn; c->params.maxLimit=mx; }
}
void ConstraintSystem::SetSpring(uint32_t id, float k, float d){
    auto* c=m_impl->Find(id);
    if(c){ c->params.stiffness=k; c->params.damping=d; }
}
float ConstraintSystem::GetAppliedForce(uint32_t id) const {
    auto* c=m_impl->Find(id); return c?c->appliedForce:0;
}
void ConstraintSystem::SetIterations(uint32_t n){ m_impl->iterations=n; }
void ConstraintSystem::SetOnBreak(uint32_t id, std::function<void()> cb){
    auto* c=m_impl->Find(id); if(c) c->onBreak=cb;
}
uint32_t ConstraintSystem::GetConstraintCount() const {
    return (uint32_t)m_impl->constraints.size();
}

void ConstraintSystem::SolveVelocity(float dt){
    for(uint32_t iter=0;iter<m_impl->iterations;iter++){
        for(auto& c:m_impl->constraints){
            if(!c.enabled) continue;
            auto itA=m_impl->bodies.find(c.bodyA);
            auto itB=m_impl->bodies.find(c.bodyB);
            if(itA==m_impl->bodies.end()||itB==m_impl->bodies.end()) continue;
            auto& A=itA->second; auto& B=itB->second;

            // Separation vector from body A to body B (world space, ignoring local anchors)
            float dx=B.pos.x-A.pos.x, dy=B.pos.y-A.pos.y, dz=B.pos.z-A.pos.z;
            float dist=std::sqrt(dx*dx+dy*dy+dz*dz);
            if(dist<1e-6f) continue;

            float restLen=c.params.restLength;
            float error=dist-restLen;
            float nx=dx/dist, ny=dy/dist, nz=dz/dist;

            // Relative velocity along constraint axis
            float rvx=B.vel.x-A.vel.x, rvy=B.vel.y-A.vel.y, rvz=B.vel.z-A.vel.z;
            float rv=rvx*nx+rvy*ny+rvz*nz;

            float sumInvMass=A.invMass+B.invMass;
            if(sumInvMass<1e-12f) continue;

            // Baumgarte bias
            float beta=0.2f;
            float jv=-rv + (beta/dt)*error;

            // Spring damping
            if(c.type==ConstraintType::Spring){
                jv += -c.params.stiffness*error*dt - c.params.damping*rv;
            }

            float lambda=jv/sumInvMass;
            c.appliedForce=std::abs(lambda);

            // Apply impulse
            A.vel.x-=A.invMass*lambda*nx; A.vel.y-=A.invMass*lambda*ny;
            A.vel.z-=A.invMass*lambda*nz;
            B.vel.x+=B.invMass*lambda*nx; B.vel.y+=B.invMass*lambda*ny;
            B.vel.z+=B.invMass*lambda*nz;

            // Check break
            if(c.appliedForce>c.breakForce){
                c.enabled=false;
                if(c.onBreak) c.onBreak();
            }
        }
    }
}

void ConstraintSystem::SolvePosition(float dt){
    for(uint32_t iter=0;iter<m_impl->iterations;iter++){
        for(auto& c:m_impl->constraints){
            if(!c.enabled) continue;
            if(c.type==ConstraintType::Spring) continue; // spring handled velocity-only
            auto itA=m_impl->bodies.find(c.bodyA);
            auto itB=m_impl->bodies.find(c.bodyB);
            if(itA==m_impl->bodies.end()||itB==m_impl->bodies.end()) continue;
            auto& A=itA->second; auto& B=itB->second;

            float dx=B.pos.x-A.pos.x, dy=B.pos.y-A.pos.y, dz=B.pos.z-A.pos.z;
            float dist=std::sqrt(dx*dx+dy*dy+dz*dz);
            if(dist<1e-6f) continue;

            float error=dist-c.params.restLength;
            if(std::abs(error)<1e-4f) continue;
            float nx=dx/dist, ny=dy/dist, nz=dz/dist;
            float sumInv=A.invMass+B.invMass;
            if(sumInv<1e-12f) continue;
            float corr=error/sumInv*0.5f;
            A.pos.x+=A.invMass*corr*nx; A.pos.y+=A.invMass*corr*ny; A.pos.z+=A.invMass*corr*nz;
            B.pos.x-=B.invMass*corr*nx; B.pos.y-=B.invMass*corr*ny; B.pos.z-=B.invMass*corr*nz;
        }
    }
    (void)dt;
}

} // namespace Engine
