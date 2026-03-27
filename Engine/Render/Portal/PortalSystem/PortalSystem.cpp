#include "Engine/Render/Portal/PortalSystem/PortalSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

struct PortalData {
    uint32_t id{0};
    float    pos[3]{};
    float    fwd[3]{0,0,1};
    float    up [3]{0,1,0};
    float    width{2.f}, height{3.f};
    uint32_t linkedId{0};
    bool     oneWay{false};
};

static void QuatFromFwd(const float fwd[3], float qOut[4]) {
    // Simple alignment to default forward (0,0,1)
    float axis[3]={-fwd[2],0,fwd[0]};
    float l=std::sqrt(axis[0]*axis[0]+axis[2]*axis[2]);
    if(l<1e-6f){qOut[0]=qOut[1]=qOut[2]=0;qOut[3]=1; return;}
    float angle=std::atan2(l,fwd[2]);
    float s=std::sin(angle*0.5f)/l;
    qOut[0]=axis[0]*s; qOut[1]=0; qOut[2]=axis[2]*s; qOut[3]=std::cos(angle*0.5f);
}

struct PortalSystem::Impl {
    std::vector<PortalData> portals;
    uint32_t nextId{1};
    uint32_t maxDepth{2};
    std::function<void(uint32_t,uint32_t)> onTeleport;

    PortalData* Find(uint32_t id){
        for(auto& p:portals) if(p.id==id) return &p; return nullptr;
    }
    const PortalData* Find(uint32_t id) const {
        for(auto& p:portals) if(p.id==id) return &p; return nullptr;
    }
};

PortalSystem::PortalSystem()  : m_impl(new Impl){}
PortalSystem::~PortalSystem() { Shutdown(); delete m_impl; }
void PortalSystem::Init()     {}
void PortalSystem::Shutdown() { m_impl->portals.clear(); }

uint32_t PortalSystem::CreatePortal(const float pos[3], const float fwd[3],
                                      float w, float h)
{
    PortalData p; p.id=m_impl->nextId++; p.width=w; p.height=h;
    if(pos) for(int i=0;i<3;i++) p.pos[i]=pos[i];
    if(fwd){float l=std::sqrt(fwd[0]*fwd[0]+fwd[1]*fwd[1]+fwd[2]*fwd[2])+1e-9f;
            for(int i=0;i<3;i++) p.fwd[i]=fwd[i]/l;}
    m_impl->portals.push_back(p);
    return m_impl->portals.back().id;
}

void PortalSystem::DestroyPortal(uint32_t id){
    auto& v=m_impl->portals;
    // Unlink partners first
    for(auto& p:v) if(p.linkedId==id) p.linkedId=0;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& p){return p.id==id;}),v.end());
}
bool     PortalSystem::HasPortal(uint32_t id) const { return m_impl->Find(id)!=nullptr; }
void     PortalSystem::Link(uint32_t a, uint32_t b){
    auto* pa=m_impl->Find(a); auto* pb=m_impl->Find(b);
    if(pa) pa->linkedId=b; if(pb) pb->linkedId=a;
}
void     PortalSystem::Unlink(uint32_t id){
    auto* p=m_impl->Find(id); if(!p) return;
    auto* other=m_impl->Find(p->linkedId);
    if(other) other->linkedId=0; p->linkedId=0;
}
uint32_t PortalSystem::LinkedPortal(uint32_t id) const {
    const auto* p=m_impl->Find(id); return p?p->linkedId:0;
}

bool PortalSystem::CheckCross(uint32_t portalId, const float prev[3], const float cur[3]) const {
    const auto* p=m_impl->Find(portalId); if(!p) return false;
    // Signed distance to portal plane
    float d0=0,d1=0;
    for(int i=0;i<3;i++){d0+=(prev[i]-p->pos[i])*p->fwd[i]; d1+=(cur[i]-p->pos[i])*p->fwd[i];}
    return (d0>0)!=(d1>0);
}

PortalTransform PortalSystem::TransformThrough(uint32_t portalId,
                                                 const float inPos[3],
                                                 const float /*inDir*/[3]) const
{
    const auto* pa=m_impl->Find(portalId); if(!pa) return {};
    const auto* pb=m_impl->Find(pa->linkedId); if(!pb) return {};
    // Translate: relative offset + exit portal pos
    PortalTransform out;
    for(int i=0;i<3;i++) out.position[i]=pb->pos[i]+(inPos[i]-pa->pos[i]);
    QuatFromFwd(pb->fwd, out.orientation);
    return out;
}

void PortalSystem::TeleportEntity(uint32_t entityId, uint32_t portalId,
                                    float inPos[3], float inDir[3])
{
    auto tr=TransformThrough(portalId,inPos,inDir);
    for(int i=0;i<3;i++) inPos[i]=tr.position[i];
    if(m_impl->onTeleport) m_impl->onTeleport(entityId, portalId);
}

PortalScissor PortalSystem::GetScissorRect(uint32_t, const float*, const float*) const {
    return {-1,-1,1,1}; // full screen (simplified)
}

PortalTransform PortalSystem::GetVirtualCamera(uint32_t portalId,
                                                 const float camPos[3],
                                                 const float camDir[3]) const {
    return TransformThrough(portalId, camPos, camDir);
}

void PortalSystem::SetOneWay(uint32_t id, bool ow){
    auto* p=m_impl->Find(id); if(p) p->oneWay=ow;
}
void PortalSystem::SetMaxRecursionDepth(uint32_t d){ m_impl->maxDepth=d; }
void PortalSystem::SetOnTeleport(std::function<void(uint32_t,uint32_t)> cb){ m_impl->onTeleport=cb; }

std::vector<uint32_t> PortalSystem::GetAll() const {
    std::vector<uint32_t> out; for(auto& p:m_impl->portals) out.push_back(p.id); return out;
}

} // namespace Engine
