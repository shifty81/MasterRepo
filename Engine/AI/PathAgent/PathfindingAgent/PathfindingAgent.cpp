#include "Engine/AI/PathAgent/PathfindingAgent/PathfindingAgent.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

static inline float Dist3(PAVec3f a, PAVec3f b){
    float dx=a.x-b.x, dy=a.y-b.y, dz=a.z-b.z;
    return std::sqrt(dx*dx+dy*dy+dz*dz);
}
static inline PAVec3f Sub3(PAVec3f a, PAVec3f b){ return {a.x-b.x,a.y-b.y,a.z-b.z}; }
static inline PAVec3f Norm3(PAVec3f v){
    float l=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
    return l>1e-6f?PAVec3f{v.x/l,v.y/l,v.z/l}:PAVec3f{0,0,0};
}

struct AgentData {
    uint32_t              id;
    float                 radius{0.5f};
    float                 maxSpeed{5.f};
    float                 arrivalRadius{0.5f};
    std::vector<PAVec3f>  waypoints;
    uint32_t              wpIdx{0};
    PAVec3f               targetPos{0,0,0};
    PAVec3f               desiredVel{0,0,0};
    PAVec3f               currentPos{0,0,0};
    AgentBehaviour        behaviour{AgentBehaviour::Idle};
    bool                  atTarget{false};
    float                 wanderAngle{0};
    std::function<void(uint32_t)> onWaypoint;
    std::function<void()>         onTarget;
};

struct PathfindingAgent::Impl {
    std::unordered_map<uint32_t,AgentData> agents;
    AgentData* Find(uint32_t id){ auto it=agents.find(id); return it!=agents.end()?&it->second:nullptr; }
};

PathfindingAgent::PathfindingAgent(): m_impl(new Impl){}
PathfindingAgent::~PathfindingAgent(){ Shutdown(); delete m_impl; }
void PathfindingAgent::Init(){}
void PathfindingAgent::Shutdown(){ m_impl->agents.clear(); }
void PathfindingAgent::Reset(){ m_impl->agents.clear(); }

void PathfindingAgent::CreateAgent(uint32_t id, float radius, float maxSpeed){
    AgentData a; a.id=id; a.radius=radius; a.maxSpeed=maxSpeed;
    m_impl->agents[id]=a;
}
void PathfindingAgent::RemoveAgent(uint32_t id){ m_impl->agents.erase(id); }

void PathfindingAgent::SetPath(uint32_t agentId, const std::vector<PAVec3f>& wp){
    auto* a=m_impl->Find(agentId); if(!a) return;
    a->waypoints=wp; a->wpIdx=0; a->atTarget=false;
}
void PathfindingAgent::SetTarget(uint32_t agentId, PAVec3f pos){
    auto* a=m_impl->Find(agentId); if(a){ a->targetPos=pos; a->atTarget=false; }
}
void PathfindingAgent::SetBehaviour(uint32_t agentId, AgentBehaviour mode){
    auto* a=m_impl->Find(agentId); if(a) a->behaviour=mode;
}

PAVec3f PathfindingAgent::Update(uint32_t agentId, PAVec3f pos, float dt){
    auto* a=m_impl->Find(agentId); if(!a) return pos;
    a->currentPos=pos;
    PAVec3f steer{0,0,0};

    switch(a->behaviour){
        case AgentBehaviour::Idle:
            a->desiredVel={0,0,0}; return pos;

        case AgentBehaviour::Arrive:
        case AgentBehaviour::Follow:{
            if(a->waypoints.empty()){ a->atTarget=true; break; }
            PAVec3f wp=a->waypoints[a->wpIdx];
            float d=Dist3(pos,wp);
            if(d<a->arrivalRadius){
                if(a->onWaypoint) a->onWaypoint(a->wpIdx);
                if(a->wpIdx+1<(uint32_t)a->waypoints.size()){
                    a->wpIdx++;
                } else {
                    a->atTarget=true;
                    if(a->onTarget) a->onTarget();
                    a->desiredVel={0,0,0}; return pos;
                }
                wp=a->waypoints[a->wpIdx];
                d=Dist3(pos,wp);
            }
            // Slow down near arrival
            float speedFactor=std::min(1.f, d/(a->arrivalRadius*3.f));
            PAVec3f dir=Norm3(Sub3(wp,pos));
            steer={dir.x*a->maxSpeed*speedFactor,
                   dir.y*a->maxSpeed*speedFactor,
                   dir.z*a->maxSpeed*speedFactor};
            break;
        }
        case AgentBehaviour::Flee:{
            PAVec3f dir=Norm3(Sub3(pos,a->targetPos));
            steer={dir.x*a->maxSpeed, dir.y*a->maxSpeed, dir.z*a->maxSpeed};
            break;
        }
        case AgentBehaviour::Wander:{
            a->wanderAngle+=((float)(rand()%200-100)/100.f)*0.3f;
            steer={std::cos(a->wanderAngle)*a->maxSpeed, 0,
                   std::sin(a->wanderAngle)*a->maxSpeed};
            break;
        }
    }

    a->desiredVel=steer;
    PAVec3f newPos{pos.x+steer.x*dt, pos.y+steer.y*dt, pos.z+steer.z*dt};
    return newPos;
}

PAVec3f  PathfindingAgent::GetDesiredVelocity(uint32_t id) const {
    auto* a=m_impl->Find(id); return a?a->desiredVel:PAVec3f{0,0,0};
}
PAVec3f  PathfindingAgent::GetCurrentWaypoint(uint32_t id) const {
    auto* a=m_impl->Find(id);
    if(!a||a->waypoints.empty()) return {0,0,0};
    return a->waypoints[a->wpIdx];
}
bool    PathfindingAgent::HasReachedTarget(uint32_t id) const {
    auto* a=m_impl->Find(id); return a&&a->atTarget;
}
float   PathfindingAgent::GetPathLength(uint32_t id) const {
    auto* a=m_impl->Find(id); if(!a||a->waypoints.empty()) return 0;
    float len=0;
    for(uint32_t i=a->wpIdx;i+1<(uint32_t)a->waypoints.size();i++)
        len+=Dist3(a->waypoints[i],a->waypoints[i+1]);
    return len;
}
uint32_t PathfindingAgent::GetWaypointIndex(uint32_t id) const {
    auto* a=m_impl->Find(id); return a?a->wpIdx:0;
}
void PathfindingAgent::SetMaxSpeed     (uint32_t id, float s){ auto* a=m_impl->Find(id); if(a) a->maxSpeed=s; }
void PathfindingAgent::SetArrivalRadius(uint32_t id, float r){ auto* a=m_impl->Find(id); if(a) a->arrivalRadius=r; }
void PathfindingAgent::SetOnWaypointReached(uint32_t id, std::function<void(uint32_t)> cb){
    auto* a=m_impl->Find(id); if(a) a->onWaypoint=cb;
}
void PathfindingAgent::SetOnTargetReached(uint32_t id, std::function<void()> cb){
    auto* a=m_impl->Find(id); if(a) a->onTarget=cb;
}

} // namespace Engine
