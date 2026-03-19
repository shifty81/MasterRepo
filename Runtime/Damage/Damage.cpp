#include "Runtime/Damage/Damage.h"
#include <algorithm>

namespace Runtime::Damage {

Health& DamageSystem::GetOrCreate(Runtime::ECS::EntityID entity) {
    return m_health[entity]; // default-constructs Health if missing
}

void DamageSystem::ApplyDamage(Runtime::ECS::EntityID entity,
                                const DamageInfo& dmg) {
    if (dmg.amount <= 0.0f)
        return;

    auto& hp = GetOrCreate(entity);
    if (hp.isDead)
        return;

    hp.current -= dmg.amount;
    if (hp.current <= 0.0f) {
        hp.current = 0.0f;
        hp.isDead  = true;
    }
}

void DamageSystem::ApplyHealing(Runtime::ECS::EntityID entity, float amount) {
    if (amount <= 0.0f)
        return;

    auto it = m_health.find(entity);
    if (it == m_health.end())
        return;

    auto& hp = it->second;
    if (hp.isDead)
        return;

    hp.current = std::min(hp.current + amount, hp.maximum);
}

void DamageSystem::Update(float dt) {
    for (auto& [id, hp] : m_health) {
        if (hp.isDead || hp.regenRate <= 0.0f)
            continue;
        hp.current = std::min(hp.current + hp.regenRate * dt, hp.maximum);
    }
}

bool DamageSystem::IsDead(Runtime::ECS::EntityID entity) const {
    auto it = m_health.find(entity);
    if (it == m_health.end())
        return false;
    return it->second.isDead;
}

void DamageSystem::Kill(Runtime::ECS::EntityID entity) {
    auto& hp   = GetOrCreate(entity);
    hp.current = 0.0f;
    hp.isDead  = true;
}

void DamageSystem::Revive(Runtime::ECS::EntityID entity, float healthPercent) {
    auto& hp  = GetOrCreate(entity);
    float pct = std::clamp(healthPercent, 0.0f, 1.0f);
    hp.current = hp.maximum * pct;
    hp.isDead  = false;
}

} // namespace Runtime::Damage
