#include "Runtime/Gameplay/Grapple/GrappleSystem/GrappleSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Runtime {

struct GrappleData {
    GrappleState_  state;
    GrappleDesc    desc;
    bool           alive{true};
};

struct GrappleSystem::Impl {
    std::vector<GrappleData> hooks;
    uint32_t nextId{1};
    GrappleHitFn hitFn;
    std::function<void(uint32_t, const float*)> onAttach;
    std::function<void(uint32_t)>               onDetach;

    GrappleData* Find(uint32_t id) {
        for(auto& h:hooks) if(h.state.id==id) return &h; return nullptr;
    }
    const GrappleData* Find(uint32_t id) const {
        for(auto& h:hooks) if(h.state.id==id) return &h; return nullptr;
    }
};

GrappleSystem::GrappleSystem()  : m_impl(new Impl) {}
GrappleSystem::~GrappleSystem() { Shutdown(); delete m_impl; }

void GrappleSystem::Init()     {}
void GrappleSystem::Shutdown() { m_impl->hooks.clear(); }

uint32_t GrappleSystem::Launch(uint32_t ownerId, const float firePos[3],
                                 const float fireDir[3], const GrappleDesc& desc)
{
    GrappleData gd; gd.desc=desc; gd.alive=true;
    gd.state.id=m_impl->nextId++; gd.state.ownerId=ownerId;
    gd.state.state=GrappleState::Flying;
    for(int i=0;i<3;i++) gd.state.hookPos[i]=firePos[i];
    float spd=desc.launchSpeed;
    for(int i=0;i<3;i++) gd.state.hookVel[i]=fireDir[i]*spd;
    m_impl->hooks.push_back(gd);
    return m_impl->hooks.back().state.id;
}

void GrappleSystem::Detach(uint32_t id) {
    auto* h=m_impl->Find(id);
    if(h) { h->state.state=GrappleState::Detached; h->alive=false;
            if(m_impl->onDetach) m_impl->onDetach(id); }
}
void GrappleSystem::Retract(uint32_t id) {
    auto* h=m_impl->Find(id); if(h) h->state.state=GrappleState::Retracting;
}
void GrappleSystem::SetMode(uint32_t id, GrappleState mode) {
    auto* h=m_impl->Find(id); if(h) h->state.state=mode;
}

const GrappleState_* GrappleSystem::Get(uint32_t id) const {
    const auto* h=m_impl->Find(id); return h?&h->state:nullptr;
}
GrappleState GrappleSystem::GetState(uint32_t id) const {
    const auto* h=m_impl->Find(id); return h?h->state.state:GrappleState::Detached;
}
bool GrappleSystem::IsAlive(uint32_t id) const {
    const auto* h=m_impl->Find(id); return h&&h->alive;
}

void GrappleSystem::GetRopeForce(uint32_t id, const float ownerPos[3], float outForce[3]) const {
    const auto* h=m_impl->Find(id);
    if(!h||h->state.state==GrappleState::Flying) { for(int i=0;i<3;i++) outForce[i]=0.f; return; }
    float delta[3];
    for(int i=0;i<3;i++) delta[i]=h->state.attachPoint[i]-ownerPos[i];
    float dist=std::sqrt(delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2]);
    if(dist<h->state.ropeLength) { for(int i=0;i<3;i++) outForce[i]=0.f; return; }
    float excess=dist-h->state.ropeLength;
    float f=h->desc.ropeStiffness*excess;
    float len=dist+1e-9f;
    for(int i=0;i<3;i++) outForce[i]=delta[i]/len*f;
}

void GrappleSystem::SetHitTestFn(GrappleHitFn fn) { m_impl->hitFn=fn; }
void GrappleSystem::SetOnAttach(std::function<void(uint32_t,const float*)> cb) { m_impl->onAttach=cb; }
void GrappleSystem::SetOnDetach(std::function<void(uint32_t)> cb) { m_impl->onDetach=cb; }

std::vector<uint32_t> GrappleSystem::GetByOwner(uint32_t ownerId) const {
    std::vector<uint32_t> out;
    for(auto& h:m_impl->hooks) if(h.state.ownerId==ownerId&&h.alive) out.push_back(h.state.id);
    return out;
}
std::vector<uint32_t> GrappleSystem::GetAll() const {
    std::vector<uint32_t> out;
    for(auto& h:m_impl->hooks) if(h.alive) out.push_back(h.state.id);
    return out;
}

void GrappleSystem::Tick(float dt)
{
    for(auto& h : m_impl->hooks) {
        if(!h.alive) continue;
        if(h.state.state==GrappleState::Flying) {
            // Apply gravity
            h.state.hookVel[1] -= h.desc.gravity * dt;
            // Advance
            float prevPos[3]; for(int i=0;i<3;i++) prevPos[i]=h.state.hookPos[i];
            for(int i=0;i<3;i++) h.state.hookPos[i]+=h.state.hookVel[i]*dt;
            h.state.travelDist += dt * h.desc.launchSpeed;
            // Hit test
            if(m_impl->hitFn) {
                float dir[3]; for(int i=0;i<3;i++) dir[i]=h.state.hookVel[i];
                float len=std::sqrt(dir[0]*dir[0]+dir[1]*dir[1]+dir[2]*dir[2])+1e-9f;
                for(int i=0;i<3;i++) dir[i]/=len;
                float hit[3];
                if(m_impl->hitFn(prevPos, dir, h.desc.launchSpeed*dt, hit)) {
                    for(int i=0;i<3;i++) h.state.attachPoint[i]=hit[i];
                    for(int i=0;i<3;i++) h.state.hookPos[i]=hit[i];
                    h.state.ropeLength = h.state.travelDist;
                    h.state.state = GrappleState::Attached;
                    if(m_impl->onAttach) m_impl->onAttach(h.state.id, hit);
                }
            }
            // Max range
            if(h.state.travelDist > h.desc.maxRange) Detach(h.state.id);
        } else if(h.state.state==GrappleState::Retracting) {
            h.state.ropeLength -= h.desc.retractSpeed * dt;
            if(h.state.ropeLength <= 0.f) Detach(h.state.id);
        }
    }
    // Remove dead hooks
    auto& v=m_impl->hooks;
    v.erase(std::remove_if(v.begin(),v.end(),[](auto& h){return !h.alive;}),v.end());
}

} // namespace Runtime
