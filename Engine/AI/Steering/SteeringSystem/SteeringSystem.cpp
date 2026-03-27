#include "Engine/AI/Steering/SteeringSystem/SteeringSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

static float Len3(const float v[3]){ return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]); }
static void Norm3(float v[3]){ float l=Len3(v)+1e-9f; v[0]/=l;v[1]/=l;v[2]/=l; }
static float Dot3(const float a[3],const float b[3]){ return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }

struct BehaviourEntry{ BehaviourType type; float weight; };
struct Obstacle{ float centre[3]; float radius; };

struct SteeringAgent {
    uint32_t id;
    SteeringAgentDesc desc;
    float pos[3]{},vel[3]{},steering[3]{};
    float target[3]{};
    uint32_t targetAgentId{0};
    std::vector<BehaviourEntry> behaviours;
    std::vector<std::vector<float>> waypoints;
    uint32_t waypointIdx{0};
    float wanderAngle{0.f};
    float wanderCircleR{2.f}, wanderCircleOff{4.f}, wanderJitter{0.3f};
    float arriveSlowR{3.f};
};

struct SteeringSystem::Impl {
    std::vector<SteeringAgent> agents;
    std::vector<Obstacle>      obstacles;
    uint32_t nextId{1};

    SteeringAgent* Find(uint32_t id){
        for(auto& a:agents) if(a.id==id) return &a; return nullptr;
    }
    const SteeringAgent* Find(uint32_t id) const {
        for(auto& a:agents) if(a.id==id) return &a; return nullptr;
    }

    void ComputeSteering(SteeringAgent& ag, float dt){
        float force[3]={};
        for(auto& be:ag.behaviours){
            float f[3]={};
            switch(be.type){
                case BehaviourType::Seek:{
                    float d[3]={ag.target[0]-ag.pos[0],ag.target[1]-ag.pos[1],ag.target[2]-ag.pos[2]};
                    float l=Len3(d)+1e-9f;
                    for(int i=0;i<3;i++) f[i]=d[i]/l*ag.desc.maxSpeed-ag.vel[i];
                } break;
                case BehaviourType::Flee:{
                    float d[3]={ag.pos[0]-ag.target[0],ag.pos[1]-ag.target[1],ag.pos[2]-ag.target[2]};
                    float l=Len3(d)+1e-9f;
                    for(int i=0;i<3;i++) f[i]=d[i]/l*ag.desc.maxSpeed-ag.vel[i];
                } break;
                case BehaviourType::Arrive:{
                    float d[3]={ag.target[0]-ag.pos[0],ag.target[1]-ag.pos[1],ag.target[2]-ag.pos[2]};
                    float dist=Len3(d)+1e-9f;
                    float speed=(dist<ag.arriveSlowR)?ag.desc.maxSpeed*(dist/ag.arriveSlowR):ag.desc.maxSpeed;
                    for(int i=0;i<3;i++) f[i]=d[i]/dist*speed-ag.vel[i];
                } break;
                case BehaviourType::Wander:{
                    ag.wanderAngle+=(((float)rand()/RAND_MAX)*2.f-1.f)*ag.wanderJitter;
                    float cx=ag.vel[0], cz=ag.vel[2];
                    float len=std::sqrt(cx*cx+cz*cz)+1e-9f; cx/=len; cz/=len;
                    float tx=cx*ag.wanderCircleOff+std::cos(ag.wanderAngle)*ag.wanderCircleR;
                    float tz=cz*ag.wanderCircleOff+std::sin(ag.wanderAngle)*ag.wanderCircleR;
                    float wander[3]={tx,0,tz}; float wl=Len3(wander)+1e-9f;
                    for(int i=0;i<3;i++) f[i]=wander[i]/wl*ag.desc.maxSpeed-ag.vel[i];
                } break;
                default: break;
            }
            // Clamp to maxForce
            float fl=Len3(f);
            if(fl>ag.desc.maxForce){ float s=ag.desc.maxForce/fl; for(int i=0;i<3;i++) f[i]*=s; }
            for(int i=0;i<3;i++) force[i]+=f[i]*be.weight;
        }
        // Clamp total
        float fl=Len3(force);
        if(fl>ag.desc.maxForce){ float s=ag.desc.maxForce/fl; for(int i=0;i<3;i++) force[i]*=s; }
        for(int i=0;i<3;i++) ag.steering[i]=force[i];
    }
};

SteeringSystem::SteeringSystem()  : m_impl(new Impl){}
SteeringSystem::~SteeringSystem() { Shutdown(); delete m_impl; }
void SteeringSystem::Init()     {}
void SteeringSystem::Shutdown() { m_impl->agents.clear(); m_impl->obstacles.clear(); }

uint32_t SteeringSystem::CreateAgent(const SteeringAgentDesc& desc, const float pos[3])
{
    SteeringAgent a; a.id=m_impl->nextId++; a.desc=desc;
    if(pos) for(int i=0;i<3;i++) a.pos[i]=pos[i];
    m_impl->agents.push_back(a); return m_impl->agents.back().id;
}
void SteeringSystem::DestroyAgent(uint32_t id){
    auto& v=m_impl->agents;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& a){return a.id==id;}),v.end());
}
bool SteeringSystem::HasAgent(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void SteeringSystem::AddBehaviour(uint32_t id, BehaviourType bt, float w){
    auto* a=m_impl->Find(id); if(a) a->behaviours.push_back({bt,w});
}
void SteeringSystem::RemoveBehaviour(uint32_t id, BehaviourType bt){
    auto* a=m_impl->Find(id); if(!a) return;
    auto& v=a->behaviours;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& e){return e.type==bt;}),v.end());
}
void SteeringSystem::ClearBehaviours(uint32_t id){
    auto* a=m_impl->Find(id); if(a) a->behaviours.clear();
}

void SteeringSystem::SetTarget(uint32_t id, const float wp[3]){
    auto* a=m_impl->Find(id); if(a) for(int i=0;i<3;i++) a->target[i]=wp[i];
}
void SteeringSystem::SetTargetAgent(uint32_t id, uint32_t tid){
    auto* a=m_impl->Find(id); if(a) a->targetAgentId=tid;
}
void SteeringSystem::SetWaypoints(uint32_t id, const std::vector<std::vector<float>>& pts){
    auto* a=m_impl->Find(id); if(a){ a->waypoints=pts; a->waypointIdx=0; }
}
void SteeringSystem::AddObstacle(const float c[3], float r){
    Obstacle o; for(int i=0;i<3;i++) o.centre[i]=c[i]; o.radius=r;
    m_impl->obstacles.push_back(o);
}
void SteeringSystem::ClearObstacles(){ m_impl->obstacles.clear(); }

void SteeringSystem::SetWanderParams(uint32_t id, float cr, float co, float j){
    auto* a=m_impl->Find(id); if(a){a->wanderCircleR=cr; a->wanderCircleOff=co; a->wanderJitter=j;}
}
void SteeringSystem::SetArriveRadius(uint32_t id, float sr){
    auto* a=m_impl->Find(id); if(a) a->arriveSlowR=sr;
}

void SteeringSystem::GetPosition(uint32_t id, float out[3]) const {
    const auto* a=m_impl->Find(id); if(a) for(int i=0;i<3;i++) out[i]=a->pos[i];
}
void SteeringSystem::GetVelocity(uint32_t id, float out[3]) const {
    const auto* a=m_impl->Find(id); if(a) for(int i=0;i<3;i++) out[i]=a->vel[i];
}
void SteeringSystem::GetSteering(uint32_t id, float out[3]) const {
    const auto* a=m_impl->Find(id); if(a) for(int i=0;i<3;i++) out[i]=a->steering[i];
}
void SteeringSystem::SetPosition(uint32_t id, const float pos[3]){
    auto* a=m_impl->Find(id); if(a) for(int i=0;i<3;i++) a->pos[i]=pos[i];
}
void SteeringSystem::SetVelocity(uint32_t id, const float vel[3]){
    auto* a=m_impl->Find(id); if(a) for(int i=0;i<3;i++) a->vel[i]=vel[i];
}

std::vector<uint32_t> SteeringSystem::GetNeighbours(uint32_t id, float radius) const {
    std::vector<uint32_t> out;
    const auto* a=m_impl->Find(id); if(!a) return out;
    for(auto& b:m_impl->agents){
        if(b.id==id) continue;
        float d[3]={b.pos[0]-a->pos[0],b.pos[1]-a->pos[1],b.pos[2]-a->pos[2]};
        if(Len3(d)<=radius) out.push_back(b.id);
    }
    return out;
}

std::vector<uint32_t> SteeringSystem::GetAll() const {
    std::vector<uint32_t> out; for(auto& a:m_impl->agents) out.push_back(a.id); return out;
}

void SteeringSystem::Tick(float dt)
{
    for(auto& ag : m_impl->agents){
        // Update target from target agent
        if(ag.targetAgentId){
            const auto* ta=m_impl->Find(ag.targetAgentId);
            if(ta) for(int i=0;i<3;i++) ag.target[i]=ta->pos[i];
        }
        m_impl->ComputeSteering(ag,dt);
        // Apply steering to velocity
        float accel=1.f/ag.desc.mass;
        for(int i=0;i<3;i++) ag.vel[i]+=ag.steering[i]*accel*dt;
        // Clamp speed
        float spd=Len3(ag.vel);
        if(spd>ag.desc.maxSpeed){ float s=ag.desc.maxSpeed/spd; for(int i=0;i<3;i++) ag.vel[i]*=s; }
        // Integrate position
        for(int i=0;i<3;i++) ag.pos[i]+=ag.vel[i]*dt;
    }
}

} // namespace Engine
