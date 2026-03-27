#include "Engine/AI/Patrol/PatrolSystem/PatrolSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

static float Dist3(const float a[3], const float b[3]){ float d[3]={a[0]-b[0],a[1]-b[1],a[2]-b[2]}; return std::sqrt(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]); }
static float Dot3 (const float a[3], const float b[3]){ return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
static float Len3 (const float v[3]){ return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])+1e-9f; }

struct Waypoint { float pos[3]{}; float wait{-1.f}; };

struct PatrolData {
    uint32_t  id; PatrolDesc desc;
    std::vector<Waypoint> wps;
    float pos[3]{}, target[3]{};
    uint32_t wpIdx{0}; int dir{1};
    float waitTimer{0};
    bool waiting{false}, alerted{false}, paused{false};
    float alertPos[3]{};
    static const std::vector<std::vector<float>> emptyWps;
};
const std::vector<std::vector<float>> PatrolData::emptyWps;

struct PatrolSystem::Impl {
    std::vector<PatrolData> patrols;
    uint32_t nextId{1};
    std::function<void(uint32_t,const float*)> onAlert;
    std::function<void(uint32_t)>              onResume;
    static const std::vector<std::vector<float>> emptyWps;

    PatrolData* Find(uint32_t id){ for(auto& p:patrols) if(p.id==id) return &p; return nullptr; }
    const PatrolData* Find(uint32_t id) const { for(auto& p:patrols) if(p.id==id) return &p; return nullptr; }
};
const std::vector<std::vector<float>> PatrolSystem::Impl::emptyWps;

PatrolSystem::PatrolSystem() : m_impl(new Impl){}
PatrolSystem::~PatrolSystem(){ Shutdown(); delete m_impl; }
void PatrolSystem::Init()     {}
void PatrolSystem::Shutdown() { m_impl->patrols.clear(); }

uint32_t PatrolSystem::Create(const PatrolDesc& d){ PatrolData pd; pd.id=m_impl->nextId++; pd.desc=d; m_impl->patrols.push_back(pd); return m_impl->patrols.back().id; }
void PatrolSystem::Destroy(uint32_t id){ auto& v=m_impl->patrols; v.erase(std::remove_if(v.begin(),v.end(),[id](auto& p){return p.id==id;}),v.end()); }
bool PatrolSystem::Has(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void PatrolSystem::AddWaypoint(uint32_t id, const float pos[3], float w){
    auto* p=m_impl->Find(id); if(!p) return;
    Waypoint wp; for(int i=0;i<3;i++) wp.pos[i]=pos[i]; wp.wait=w;
    p->wps.push_back(wp);
    if(p->wps.size()==1) for(int i=0;i<3;i++) p->target[i]=pos[i];
}
void PatrolSystem::ClearWaypoints(uint32_t id){ auto* p=m_impl->Find(id); if(p) p->wps.clear(); }

const std::vector<std::vector<float>>& PatrolSystem::GetWaypoints(uint32_t id) const {
    // Return empty placeholder — caller should iterate differently; this is simplified
    static std::vector<std::vector<float>> dummy;
    auto* p=m_impl->Find(id); if(!p) return dummy;
    dummy.clear(); for(auto& w:p->wps){ dummy.push_back({w.pos[0],w.pos[1],w.pos[2]}); } return dummy;
}

void PatrolSystem::UpdatePos(uint32_t id, const float pos[3]){ auto* p=m_impl->Find(id); if(p) for(int i=0;i<3;i++) p->pos[i]=pos[i]; }
const float* PatrolSystem::GetCurrentPos   (uint32_t id) const { const auto* p=m_impl->Find(id); return p?p->pos:nullptr; }
const float* PatrolSystem::GetCurrentTarget(uint32_t id) const { const auto* p=m_impl->Find(id); return p?p->target:nullptr; }
bool PatrolSystem::IsWaiting(uint32_t id) const { const auto* p=m_impl->Find(id); return p&&p->waiting; }
bool PatrolSystem::IsAlerted(uint32_t id) const { const auto* p=m_impl->Find(id); return p&&p->alerted; }
bool PatrolSystem::IsPaused (uint32_t id) const { const auto* p=m_impl->Find(id); return p&&p->paused; }
void PatrolSystem::Pause (uint32_t id){ auto* p=m_impl->Find(id); if(p) p->paused=true; }
void PatrolSystem::Resume(uint32_t id){ auto* p=m_impl->Find(id); if(p) p->paused=false; }

void PatrolSystem::AlertTarget(uint32_t id, const float tp[3]){
    auto* p=m_impl->Find(id); if(!p) return;
    p->alerted=true; for(int i=0;i<3;i++){p->alertPos[i]=tp[i]; p->target[i]=tp[i];}
    if(m_impl->onAlert) m_impl->onAlert(id,tp);
}
void PatrolSystem::ClearAlert(uint32_t id){
    auto* p=m_impl->Find(id); if(!p) return;
    bool was=p->alerted; p->alerted=false;
    if(was&&m_impl->onResume) m_impl->onResume(id);
}

bool PatrolSystem::PointInFOV(uint32_t id, const float pt[3]) const {
    const auto* p=m_impl->Find(id); if(!p||p->wps.empty()) return false;
    float toTarget[3]={p->target[0]-p->pos[0],p->target[1]-p->pos[1],p->target[2]-p->pos[2]};
    float toPt    [3]={pt[0]-p->pos[0],pt[1]-p->pos[1],pt[2]-p->pos[2]};
    float dist=Len3(toPt); if(dist>p->desc.fovRange) return false;
    float lT=Len3(toTarget); if(lT<1e-6f) return false;
    float cos=Dot3(toTarget,toPt)/(lT*dist);
    float halfRad=p->desc.fovAngle*3.14159f/180.f;
    return cos>=std::cos(halfRad);
}

void PatrolSystem::SetOnAlert       (std::function<void(uint32_t,const float*)> cb){ m_impl->onAlert=cb; }
void PatrolSystem::SetOnResumePatrol(std::function<void(uint32_t)> cb)             { m_impl->onResume=cb; }
std::vector<uint32_t> PatrolSystem::GetAll() const { std::vector<uint32_t> v; for(auto& p:m_impl->patrols) v.push_back(p.id); return v; }

void PatrolSystem::Tick(float dt){
    for(auto& pd:m_impl->patrols){
        if(pd.paused||pd.wps.empty()) continue;
        if(pd.alerted) continue; // freeze patrol while alerted
        if(pd.waiting){
            float wt=pd.wps[pd.wpIdx].wait<0?pd.desc.waitTime:pd.wps[pd.wpIdx].wait;
            pd.waitTimer+=dt;
            if(pd.waitTimer>=wt){ pd.waiting=false; pd.waitTimer=0;
                // Advance waypoint
                switch(pd.desc.mode){
                    case PatrolMode::Loop: pd.wpIdx=(pd.wpIdx+1)%(uint32_t)pd.wps.size(); break;
                    case PatrolMode::PingPong:
                        pd.wpIdx=(uint32_t)((int32_t)pd.wpIdx+pd.dir);
                        if(pd.wpIdx==0||(uint32_t)pd.wpIdx==pd.wps.size()-1) pd.dir=-pd.dir; break;
                    case PatrolMode::Random:
                        pd.wpIdx=(uint32_t)(std::rand()%pd.wps.size()); break;
                }
                for(int i=0;i<3;i++) pd.target[i]=pd.wps[pd.wpIdx].pos[i];
            }
            continue;
        }
        // Move toward target
        float d[3]={pd.target[0]-pd.pos[0],pd.target[1]-pd.pos[1],pd.target[2]-pd.pos[2]};
        float dist=Len3(d);
        float step=pd.desc.speed*dt;
        if(dist<=step){ for(int i=0;i<3;i++) pd.pos[i]=pd.target[i]; pd.waiting=true; }
        else { for(int i=0;i<3;i++) pd.pos[i]+=d[i]/dist*step; }
    }
}

} // namespace Engine
