#include "Runtime/Gameplay/AoE/AreaOfEffectSystem/AreaOfEffectSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Runtime {

static float Dist3(AoEVec3 a, AoEVec3 b){
    float dx=a.x-b.x, dy=a.y-b.y, dz=a.z-b.z;
    return std::sqrt(dx*dx+dy*dy+dz*dz);
}
static float Dot2(float ax,float az,float bx,float bz){ return ax*bx+az*bz; }
static float Len2(float x,float z){ return std::sqrt(x*x+z*z); }

struct TickCB { float interval; float timer; std::function<void(uint32_t)> cb; };

struct AoEZone {
    uint32_t  id;
    AoEShape  shape;
    AoEVec3   pos{};
    float     yaw{0};
    AoEParams params;
    std::vector<uint32_t> inside; // agents currently inside
    std::function<void(uint32_t)> onEnter;
    std::function<void(uint32_t)> onExit;
    std::vector<TickCB> tickCBs;

    bool Contains(AoEVec3 pt) const {
        float dx=pt.x-pos.x, dz=pt.z-pos.z;
        float distXZ=std::sqrt(dx*dx+dz*dz);
        float dy=std::abs(pt.y-pos.y);
        switch(shape){
            case AoEShape::Sphere:
                return Dist3(pt,pos)<=params.radius;
            case AoEShape::Cylinder:
                return distXZ<=params.radius&&dy<=params.height*0.5f;
            case AoEShape::Cone: {
                // Rotate dx/dz by -yaw to get local space
                float yRad=yaw*3.14159f/180.f;
                float lx=dx*std::cos(-yRad)-dz*std::sin(-yRad);
                float lz=dx*std::sin(-yRad)+dz*std::cos(-yRad);
                if(lz<0) return false; // behind cone origin
                float halfA=params.angle*3.14159f/180.f;
                float coneAngle=std::atan2(std::sqrt(lx*lx),lz);
                return coneAngle<=halfA&&distXZ<=params.radius;
            }
            case AoEShape::Ring:
                return distXZ>=params.innerRadius&&distXZ<=params.radius&&dy<=params.height*0.5f;
        }
        return false;
    }
};

struct AgentPos { uint32_t id; AoEVec3 pos; };

struct AreaOfEffectSystem::Impl {
    std::vector<AoEZone> zones;
    std::vector<AgentPos> agents;

    AoEZone* Find(uint32_t id){
        for(auto& z:zones) if(z.id==id) return &z; return nullptr;
    }
    AgentPos* FindAgent(uint32_t id){
        for(auto& a:agents) if(a.id==id) return &a; return nullptr;
    }
};

AreaOfEffectSystem::AreaOfEffectSystem(): m_impl(new Impl){}
AreaOfEffectSystem::~AreaOfEffectSystem(){ Shutdown(); delete m_impl; }
void AreaOfEffectSystem::Init(){}
void AreaOfEffectSystem::Shutdown(){ m_impl->zones.clear(); m_impl->agents.clear(); }
void AreaOfEffectSystem::Reset(){ Shutdown(); }

uint32_t AreaOfEffectSystem::CreateAoE(uint32_t id, AoEShape shape,
                                        AoEVec3 pos, const AoEParams& params){
    AoEZone z; z.id=id; z.shape=shape; z.pos=pos; z.params=params;
    m_impl->zones.push_back(z); return id;
}
void AreaOfEffectSystem::DestroyAoE(uint32_t id){
    m_impl->zones.erase(std::remove_if(m_impl->zones.begin(),m_impl->zones.end(),
        [id](const AoEZone& z){return z.id==id;}),m_impl->zones.end());
}
void AreaOfEffectSystem::SetPosition(uint32_t id, AoEVec3 pos){
    auto* z=m_impl->Find(id); if(z) z->pos=pos;
}
void AreaOfEffectSystem::SetRotation(uint32_t id, float yaw){
    auto* z=m_impl->Find(id); if(z) z->yaw=yaw;
}
void AreaOfEffectSystem::SetRadius(uint32_t id, float r){
    auto* z=m_impl->Find(id); if(z) z->params.radius=r;
}
void AreaOfEffectSystem::SetHeight(uint32_t id, float h){
    auto* z=m_impl->Find(id); if(z) z->params.height=h;
}
void AreaOfEffectSystem::SetAngle(uint32_t id, float deg){
    auto* z=m_impl->Find(id); if(z) z->params.angle=deg;
}

void AreaOfEffectSystem::RegisterAgent  (uint32_t agentId, AoEVec3 pos){ m_impl->agents.push_back({agentId,pos}); }
void AreaOfEffectSystem::UpdateAgent    (uint32_t agentId, AoEVec3 pos){ auto* a=m_impl->FindAgent(agentId); if(a) a->pos=pos; }
void AreaOfEffectSystem::UnregisterAgent(uint32_t agentId){
    m_impl->agents.erase(std::remove_if(m_impl->agents.begin(),m_impl->agents.end(),
        [agentId](const AgentPos& a){return a.id==agentId;}),m_impl->agents.end());
}

void AreaOfEffectSystem::Tick(float dt){
    for(auto& z:m_impl->zones){
        std::vector<uint32_t> nowInside;
        for(auto& a:m_impl->agents){
            if(z.Contains(a.pos)) nowInside.push_back(a.id);
        }
        // Enter/Exit events
        for(auto aid:nowInside){
            bool wasInside=std::find(z.inside.begin(),z.inside.end(),aid)!=z.inside.end();
            if(!wasInside&&z.onEnter) z.onEnter(aid);
        }
        for(auto aid:z.inside){
            bool stillInside=std::find(nowInside.begin(),nowInside.end(),aid)!=nowInside.end();
            if(!stillInside&&z.onExit) z.onExit(aid);
        }
        z.inside=nowInside;
        // Tick callbacks
        for(auto& tc:z.tickCBs){
            tc.timer-=dt;
            if(tc.timer<=0){
                tc.timer=tc.interval;
                for(auto aid:z.inside) if(tc.cb) tc.cb(aid);
            }
        }
    }
}

uint32_t AreaOfEffectSystem::Query(AoEVec3 pos, std::vector<uint32_t>& out) const {
    out.clear();
    for(auto& z:m_impl->zones) if(z.Contains(pos)) out.push_back(z.id);
    return (uint32_t)out.size();
}
std::vector<uint32_t> AreaOfEffectSystem::GetOverlappingAgents(uint32_t id) const {
    auto* z=m_impl->Find(id); return z?z->inside:std::vector<uint32_t>{};
}
uint32_t AreaOfEffectSystem::GetAoECount() const { return (uint32_t)m_impl->zones.size(); }

void AreaOfEffectSystem::SetOnEnter(uint32_t id, std::function<void(uint32_t)> cb){
    auto* z=m_impl->Find(id); if(z) z->onEnter=cb;
}
void AreaOfEffectSystem::SetOnExit(uint32_t id, std::function<void(uint32_t)> cb){
    auto* z=m_impl->Find(id); if(z) z->onExit=cb;
}
void AreaOfEffectSystem::SetOnTick(uint32_t id, float interval,
    std::function<void(uint32_t)> cb){
    auto* z=m_impl->Find(id); if(!z) return;
    TickCB tc; tc.interval=interval; tc.timer=interval; tc.cb=cb;
    z->tickCBs.push_back(tc);
}

} // namespace Runtime
