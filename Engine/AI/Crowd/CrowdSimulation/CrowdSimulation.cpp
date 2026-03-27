#include "Engine/AI/Crowd/CrowdSimulation/CrowdSimulation.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

static float Len2(float x, float z){ return std::sqrt(x*x+z*z); }
static CrowdVec3 Norm2(float x, float z, float scale=1){
    float l=Len2(x,z); return l>0?CrowdVec3{x/l*scale,0,z/l*scale}:CrowdVec3{};
}

struct Agent {
    uint32_t  id;
    CrowdVec3 pos{};
    CrowdVec3 vel{};
    CrowdVec3 preferred{};
    CrowdVec3 goal{};
    bool      hasGoal{false};
    float     maxSpeed{3.5f};
    float     radius{0.4f};
};

struct CrowdSimulation::Impl {
    std::vector<Agent>     agents;
    float                  arriveThreshold{0.5f};
    std::function<void(uint32_t)> onArrive;

    Agent* Find(uint32_t id){
        for(auto& a:agents) if(a.id==id) return &a; return nullptr;
    }

    // ORCA-lite: simple velocity obstacle avoidance
    CrowdVec3 ComputeAvoidance(const Agent& a, float dt) const {
        float px=a.preferred.x, pz=a.preferred.z;
        for(auto& b:agents){
            if(b.id==a.id) continue;
            float dx=b.pos.x-a.pos.x, dz=b.pos.z-a.pos.z;
            float dist=Len2(dx,dz);
            float minDist=a.radius+b.radius+0.1f;
            if(dist<minDist&&dist>0){
                float avoid=-minDist/dist;
                px+=dx*avoid*0.5f;
                pz+=dz*avoid*0.5f;
            }
        }
        // Clamp to max speed
        float spd=Len2(px,pz);
        if(spd>a.maxSpeed) { float s=a.maxSpeed/spd; px*=s; pz*=s; }
        return {px, 0, pz};
    }
};

CrowdSimulation::CrowdSimulation(): m_impl(new Impl){}
CrowdSimulation::~CrowdSimulation(){ Shutdown(); delete m_impl; }
void CrowdSimulation::Init(){}
void CrowdSimulation::Shutdown(){ m_impl->agents.clear(); }
void CrowdSimulation::Reset(){ m_impl->agents.clear(); }

bool CrowdSimulation::AddAgent(uint32_t id, CrowdVec3 pos, float r, float ms){
    if(m_impl->Find(id)) return false;
    Agent a; a.id=id; a.pos=pos; a.radius=r; a.maxSpeed=ms;
    m_impl->agents.push_back(a); return true;
}
void CrowdSimulation::RemoveAgent(uint32_t id){
    m_impl->agents.erase(std::remove_if(m_impl->agents.begin(),m_impl->agents.end(),
        [id](const Agent& a){return a.id==id;}),m_impl->agents.end());
}
uint32_t CrowdSimulation::GetAgentCount() const { return (uint32_t)m_impl->agents.size(); }

void CrowdSimulation::SetGoal(uint32_t id, CrowdVec3 goal){
    auto* a=m_impl->Find(id); if(!a) return;
    a->goal=goal; a->hasGoal=true;
}
void CrowdSimulation::ClearGoal(uint32_t id){
    auto* a=m_impl->Find(id); if(a){ a->hasGoal=false; a->preferred={}; }
}
void CrowdSimulation::SetPreferredVelocity(uint32_t id, CrowdVec3 vel){
    auto* a=m_impl->Find(id); if(a) a->preferred=vel;
}
void CrowdSimulation::SetAgentMaxSpeed(uint32_t id, float v){
    auto* a=m_impl->Find(id); if(a) a->maxSpeed=v;
}
void CrowdSimulation::SetAgentRadius(uint32_t id, float r){
    auto* a=m_impl->Find(id); if(a) a->radius=r;
}
CrowdVec3 CrowdSimulation::GetAgentPos(uint32_t id) const {
    auto* a=m_impl->Find(id); return a?a->pos:CrowdVec3{};
}
CrowdVec3 CrowdSimulation::GetAgentVelocity(uint32_t id) const {
    auto* a=m_impl->Find(id); return a?a->vel:CrowdVec3{};
}
float CrowdSimulation::GetDensityAt(CrowdVec3 pos, float radius) const {
    uint32_t count=0;
    for(auto& a:m_impl->agents){
        float dx=a.pos.x-pos.x, dz=a.pos.z-pos.z;
        if(Len2(dx,dz)<=radius) count++;
    }
    float area=3.14159f*radius*radius;
    return area>0?(float)count/area:0;
}

void CrowdSimulation::AddStaticObstacle  (const CrowdVec3*, uint32_t){}
void CrowdSimulation::ClearStaticObstacles(){}

void CrowdSimulation::Step(float dt){
    // Compute preferred velocities from goals
    for(auto& a:m_impl->agents){
        if(a.hasGoal){
            float dx=a.goal.x-a.pos.x, dz=a.goal.z-a.pos.z;
            float d=Len2(dx,dz);
            if(d<m_impl->arriveThreshold){
                a.hasGoal=false; a.preferred={};
                if(m_impl->onArrive) m_impl->onArrive(a.id);
            } else {
                float s=std::min(a.maxSpeed, d*3.f);
                a.preferred=Norm2(dx,dz,s);
            }
        }
    }
    // Avoidance + integrate
    std::vector<CrowdVec3> newVels(m_impl->agents.size());
    for(size_t i=0;i<m_impl->agents.size();i++)
        newVels[i]=m_impl->ComputeAvoidance(m_impl->agents[i],dt);
    for(size_t i=0;i<m_impl->agents.size();i++){
        m_impl->agents[i].vel=newVels[i];
        m_impl->agents[i].pos.x+=newVels[i].x*dt;
        m_impl->agents[i].pos.z+=newVels[i].z*dt;
    }
}
void CrowdSimulation::SetOnAgentArrive(std::function<void(uint32_t)> cb){ m_impl->onArrive=cb; }

} // namespace Engine
