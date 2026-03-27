#include "Runtime/Gameplay/TrapSystem/TrapSystem/TrapSystem.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace Runtime {

struct TrapState {
    uint32_t  id{0};
    TrapDesc  desc;
    bool      armed{true};
    bool      revealed{false};
    float     cooldownTimer{0.f};
    float     activationTimer{0.f};
    bool      pendingActivate{false};
    uint32_t  pendingTargetId{0};
};

struct TrapSystem::Impl {
    std::vector<TrapState> traps;
    uint32_t nextId{1};
    ActivateCb onActivate;
    std::function<void(uint32_t)> onDetected;

    TrapState* Find(uint32_t id) {
        for (auto& t:traps) if(t.id==id) return &t; return nullptr;
    }
    const TrapState* Find(uint32_t id) const {
        for (auto& t:traps) if(t.id==id) return &t; return nullptr;
    }

    bool InZone(const TrapState& t, const float pos[3]) const {
        float dx=pos[0]-t.desc.position[0];
        float dy=pos[1]-t.desc.position[1];
        float dz=pos[2]-t.desc.position[2];
        if (t.desc.useAABB) {
            return std::abs(dx)<t.desc.triggerHalf[0] &&
                   std::abs(dy)<t.desc.triggerHalf[1] &&
                   std::abs(dz)<t.desc.triggerHalf[2];
        }
        return dx*dx+dy*dy+dz*dz < t.desc.triggerRadius*t.desc.triggerRadius;
    }
};

TrapSystem::TrapSystem()  : m_impl(new Impl) {}
TrapSystem::~TrapSystem() { Shutdown(); delete m_impl; }

void TrapSystem::Init()     {}
void TrapSystem::Shutdown() { m_impl->traps.clear(); }

uint32_t TrapSystem::Place(const TrapDesc& desc) {
    TrapState ts; ts.id=m_impl->nextId++; ts.desc=desc;
    m_impl->traps.push_back(ts); return ts.id;
}
void TrapSystem::Remove(uint32_t id) {
    auto& v=m_impl->traps;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& t){return t.id==id;}),v.end());
}
bool TrapSystem::Has(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void  TrapSystem::Arm   (uint32_t id) { auto* t=m_impl->Find(id); if(t) t->armed=true; }
void  TrapSystem::Disarm(uint32_t id) { auto* t=m_impl->Find(id); if(t) t->armed=false; }
bool  TrapSystem::IsArmed(uint32_t id) const { const auto* t=m_impl->Find(id); return t&&t->armed; }
bool  TrapSystem::IsCoolingDown(uint32_t id) const { const auto* t=m_impl->Find(id); return t&&t->cooldownTimer>0.f; }
float TrapSystem::CooldownRemaining(uint32_t id) const { const auto* t=m_impl->Find(id); return t?t->cooldownTimer:0.f; }

bool TrapSystem::DetectTrap(uint32_t id, float skill) const {
    const auto* t=m_impl->Find(id); if(!t||!t->desc.hidden) return true;
    float chance = t->desc.detectionChance * skill;
    return (float)std::rand()/(float)RAND_MAX < chance;
}
void TrapSystem::Reveal(uint32_t id) { auto* t=m_impl->Find(id); if(t) t->revealed=true; }

void TrapSystem::TriggerCheck(uint32_t id, const float actorPos[3], uint32_t actorId) {
    auto* t=m_impl->Find(id);
    if (!t||!t->armed||t->cooldownTimer>0.f) return;
    if (m_impl->InZone(*t, actorPos)) ForceActivate(id, actorId);
}

void TrapSystem::ForceActivate(uint32_t id, uint32_t targetId) {
    auto* t=m_impl->Find(id);
    if (!t||t->cooldownTimer>0.f) return;
    TrapEvent ev; ev.trapId=id; ev.type=t->desc.type;
    for(int i=0;i<3;i++) ev.position[i]=t->desc.position[i];
    ev.damage=t->desc.damage; ev.targetId=targetId;
    if (m_impl->onActivate) m_impl->onActivate(ev);
    t->cooldownTimer = t->desc.cooldown;
    if (t->desc.oneShot) Remove(id);
}

void TrapSystem::SetOnActivate(ActivateCb cb) { m_impl->onActivate=cb; }
void TrapSystem::SetOnDetected(std::function<void(uint32_t)> cb) { m_impl->onDetected=cb; }

std::vector<uint32_t> TrapSystem::GetAll() const {
    std::vector<uint32_t> out; for(auto& t:m_impl->traps) out.push_back(t.id); return out;
}
std::vector<uint32_t> TrapSystem::GetArmed() const {
    std::vector<uint32_t> out; for(auto& t:m_impl->traps) if(t.armed) out.push_back(t.id); return out;
}
std::vector<uint32_t> TrapSystem::GetNearby(const float pos[3], float radius) const {
    std::vector<uint32_t> out;
    for (auto& t:m_impl->traps) {
        float dx=pos[0]-t.desc.position[0], dy=pos[1]-t.desc.position[1], dz=pos[2]-t.desc.position[2];
        if (dx*dx+dy*dy+dz*dz<radius*radius) out.push_back(t.id);
    }
    return out;
}
const TrapDesc* TrapSystem::GetDesc(uint32_t id) const {
    const auto* t=m_impl->Find(id); return t?&t->desc:nullptr;
}

void TrapSystem::Tick(float dt) {
    for (auto& t : m_impl->traps) {
        if (t.cooldownTimer > 0.f) {
            t.cooldownTimer -= dt;
            if (t.cooldownTimer < 0.f) t.cooldownTimer = 0.f;
        }
    }
}

} // namespace Runtime
