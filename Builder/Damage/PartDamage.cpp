#include "Builder/Damage/PartDamage.h"
#include <algorithm>

namespace Builder {

void BuilderDamageSystem::InitFromAssembly(const Assembly& assembly, const PartLibrary& library) {
    m_states.clear();
    for (uint32_t id = 1; id <= static_cast<uint32_t>(assembly.PartCount() * 2 + 1); ++id) {
        const PlacedPart* pp = assembly.GetPart(id);
        if (!pp) continue;
        const PartDef* def = library.Get(pp->defId);
        float hp = def ? def->stats.hitpoints : 100.0f;
        PartHealthState s;
        s.instanceId = pp->instanceId;
        s.currentHP  = hp;
        s.maxHP      = hp;
        s.destroyed  = false;
        m_states[pp->instanceId] = s;
    }
}

PartHealthState BuilderDamageSystem::ApplyDamage(const DamageEvent& evt) {
    auto it = m_states.find(evt.targetInstanceId);
    if (it == m_states.end()) return {};
    PartHealthState& s = it->second;
    if (s.destroyed) return s;
    s.currentHP -= evt.amount;
    if (s.currentHP <= 0.0f) {
        s.currentHP = 0.0f;
        s.destroyed = true;
        s.effects.push_back("destroyed");
    }
    return s;
}

void BuilderDamageSystem::RepairPart(uint32_t instanceId, float amount) {
    auto it = m_states.find(instanceId);
    if (it == m_states.end()) return;
    PartHealthState& s = it->second;
    s.currentHP = std::min(s.maxHP, s.currentHP + amount);
    if (s.currentHP > 0.0f) {
        s.destroyed = false;
        s.effects.erase(std::remove(s.effects.begin(), s.effects.end(), std::string("destroyed")),
                        s.effects.end());
    }
}

bool BuilderDamageSystem::IsDestroyed(uint32_t instanceId) const {
    auto it = m_states.find(instanceId);
    return it != m_states.end() && it->second.destroyed;
}

PartHealthState* BuilderDamageSystem::GetState(uint32_t instanceId) {
    auto it = m_states.find(instanceId);
    return it != m_states.end() ? &it->second : nullptr;
}
const PartHealthState* BuilderDamageSystem::GetState(uint32_t instanceId) const {
    auto it = m_states.find(instanceId);
    return it != m_states.end() ? &it->second : nullptr;
}

std::vector<uint32_t> BuilderDamageSystem::GetDestroyedParts() const {
    std::vector<uint32_t> out;
    for (const auto& [id, s] : m_states)
        if (s.destroyed) out.push_back(id);
    return out;
}

void BuilderDamageSystem::Reset() { m_states.clear(); }

} // namespace Builder
