#include "Runtime/Crowd/CrowdSimulation.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Runtime {

static float Dist3(const float* a, const float* b){
    float dx=a[0]-b[0],dy=a[1]-b[1],dz=a[2]-b[2]; return std::sqrt(dx*dx+dy*dy+dz*dz);
}
static void Normalize3(float v[3]){
    float l=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]); if(l>1e-6f){v[0]/=l;v[1]/=l;v[2]/=l;}
}

struct CrowdSimulation::Impl {
    CrowdConfig cfg;
    std::unordered_map<uint32_t,CrowdAgent>    agents;
    std::unordered_map<uint32_t,CrowdFormation> formations;
    uint32_t nextAgentId{1}, nextFormationId{1};
    void*    navMesh{nullptr};

    std::function<void(uint32_t)> onGoalReached;
    std::function<void(uint32_t)> onBlocked;
};

CrowdSimulation::CrowdSimulation() : m_impl(new Impl()) {}
CrowdSimulation::~CrowdSimulation() { delete m_impl; }

void CrowdSimulation::Init(const CrowdConfig& cfg) { m_impl->cfg=cfg; }
void CrowdSimulation::Shutdown() { m_impl->agents.clear(); }
void CrowdSimulation::SetNavMeshPtr(void* nm) { m_impl->navMesh=nm; }

uint32_t CrowdSimulation::SpawnAgent(const float pos[3], const float goal[3]) {
    uint32_t id=m_impl->nextAgentId++;
    CrowdAgent& a=m_impl->agents[id];
    a.id=id;
    for(int i=0;i<3;++i){ a.position[i]=pos[i]; a.goalPosition[i]=goal[i]; }
    a.speed=m_impl->cfg.agentSpeed;
    a.radius=m_impl->cfg.agentRadius;
    return id;
}

void CrowdSimulation::DestroyAgent(uint32_t id) { m_impl->agents.erase(id); }

void CrowdSimulation::SetAgentGoal(uint32_t id, const float goal[3]) {
    auto it=m_impl->agents.find(id);
    if(it!=m_impl->agents.end()){ for(int i=0;i<3;++i) it->second.goalPosition[i]=goal[i]; it->second.reachedGoal=false; }
}

void CrowdSimulation::SetAgentSpeed(uint32_t id, float s) {
    auto it=m_impl->agents.find(id); if(it!=m_impl->agents.end()) it->second.speed=s;
}

CrowdAgent CrowdSimulation::GetAgent(uint32_t id) const {
    auto it=m_impl->agents.find(id); return it!=m_impl->agents.end()?it->second:CrowdAgent{};
}

std::vector<CrowdAgent> CrowdSimulation::AllAgents() const {
    std::vector<CrowdAgent> out; for(auto&[k,v]:m_impl->agents) out.push_back(v); return out;
}
uint32_t CrowdSimulation::ActiveAgentCount() const { return (uint32_t)m_impl->agents.size(); }

uint32_t CrowdSimulation::CreateFormation(const std::string& name) {
    uint32_t id=m_impl->nextFormationId++;
    m_impl->formations[id].id=id; m_impl->formations[id].name=name;
    return id;
}
void CrowdSimulation::AddAgentToFormation(uint32_t fid, uint32_t aid) {
    auto it=m_impl->formations.find(fid);
    if(it!=m_impl->formations.end()){ it->second.agentIds.push_back(aid);
        auto ait=m_impl->agents.find(aid); if(ait!=m_impl->agents.end()) ait->second.formationGroupId=fid; }
}
void CrowdSimulation::SetFormationGoal(uint32_t fid, const float goal[3]) {
    auto it=m_impl->formations.find(fid); if(it==m_impl->formations.end()) return;
    for(uint32_t aid:it->second.agentIds) SetAgentGoal(aid,goal);
}
void CrowdSimulation::DisbandFormation(uint32_t fid) {
    auto it=m_impl->formations.find(fid); if(it==m_impl->formations.end()) return;
    for(uint32_t aid:it->second.agentIds){
        auto ait=m_impl->agents.find(aid); if(ait!=m_impl->agents.end()) ait->second.formationGroupId=0;
    }
    m_impl->formations.erase(fid);
}

void CrowdSimulation::Update(float dt) {
    for(auto&[id,a]:m_impl->agents){
        if(!a.active||a.reachedGoal) continue;
        // Steer toward goal
        float dir[3]={a.goalPosition[0]-a.position[0],
                      a.goalPosition[1]-a.position[1],
                      a.goalPosition[2]-a.position[2]};
        float dist=Dist3(a.position,a.goalPosition);
        if(dist<a.radius*0.5f){ a.reachedGoal=true; if(m_impl->onGoalReached) m_impl->onGoalReached(id); continue; }
        Normalize3(dir);
        // Simple separation from neighbours
        float sep[3]={0,0,0};
        for(auto&[oid,oa]:m_impl->agents){
            if(oid==id) continue;
            float d=Dist3(a.position,oa.position);
            if(d<a.radius*2.f&&d>1e-6f){
                float push[3]={a.position[0]-oa.position[0],a.position[1]-oa.position[1],a.position[2]-oa.position[2]};
                float scale=(a.radius*2.f-d)/(a.radius*2.f);
                sep[0]+=push[0]*scale; sep[1]+=push[1]*scale; sep[2]+=push[2]*scale;
            }
        }
        float stepSpeed=a.speed*dt;
        for(int i=0;i<3;++i){
            a.velocity[i]=dir[i]*stepSpeed+sep[i]*m_impl->cfg.separationWeight*dt;
            a.position[i]+=a.velocity[i];
        }
    }
}

std::vector<uint32_t> CrowdSimulation::AgentsInRadius(const float c[3], float r) const {
    std::vector<uint32_t> out;
    for(auto&[id,a]:m_impl->agents) if(Dist3(a.position,c)<=r) out.push_back(id);
    return out;
}

void CrowdSimulation::OnAgentGoalReached(std::function<void(uint32_t)> cb) { m_impl->onGoalReached=std::move(cb); }
void CrowdSimulation::OnAgentBlocked(std::function<void(uint32_t)> cb)     { m_impl->onBlocked=std::move(cb); }

} // namespace Runtime
