#include "Game/Gameplay/Combat/CombatSystem.h"
#include <algorithm>

namespace NF::Game::Gameplay {

// ---- private helpers --------------------------------------------------------

CombatSystem::Entry* CombatSystem::FindEntry(uint32_t id) noexcept {
    for (auto& e : m_Entities) {
        if (e.id == id) return &e;
    }
    return nullptr;
}

const CombatSystem::Entry* CombatSystem::FindEntry(uint32_t id) const noexcept {
    for (const auto& e : m_Entities) {
        if (e.id == id) return &e;
    }
    return nullptr;
}

// ---- entity registration ----------------------------------------------------

void CombatSystem::RegisterEntity(uint32_t entityId, float maxHealth, float armor) {
    if (FindEntry(entityId)) return; // already registered
    CombatState state;
    state.health    = maxHealth;
    state.maxHealth = maxHealth;
    state.armor     = armor;
    state.alive     = true;
    m_Entities.push_back({ entityId, state });
}

bool CombatSystem::UnregisterEntity(uint32_t entityId) {
    auto it = std::find_if(m_Entities.begin(), m_Entities.end(),
                           [entityId](const Entry& e){ return e.id == entityId; });
    if (it == m_Entities.end()) return false;
    m_Entities.erase(it);
    return true;
}

// ---- combat -----------------------------------------------------------------

DamageResult CombatSystem::DealDamage(uint32_t entityId, float amount, DamageType type) {
    Entry* e = FindEntry(entityId);
    if (!e) return DamageResult::NotFound;
    if (!e->state.alive) return DamageResult::AlreadyDead;

    // Mitigation: armor only reduces physical damage.
    float effective = amount;
    if (type == DamageType::Physical) {
        effective -= e->state.armor;
        if (effective < 0.f) effective = 0.f;
    }

    e->state.health -= effective;
    if (e->state.health < 0.f) e->state.health = 0.f;

    if (e->state.health == 0.f && e->state.alive) {
        e->state.alive = false;
        if (m_DeathCb) m_DeathCb(entityId);
        return DamageResult::Killed;
    }
    return DamageResult::Hit;
}

bool CombatSystem::Heal(uint32_t entityId, float amount) {
    Entry* e = FindEntry(entityId);
    if (!e || !e->state.alive) return false;
    e->state.health += amount;
    if (e->state.health > e->state.maxHealth)
        e->state.health = e->state.maxHealth;
    return true;
}

// ---- state queries ----------------------------------------------------------

bool CombatSystem::IsAlive(uint32_t entityId) const noexcept {
    const Entry* e = FindEntry(entityId);
    return e ? e->state.alive : false;
}

CombatState CombatSystem::GetState(uint32_t entityId) const noexcept {
    const Entry* e = FindEntry(entityId);
    if (!e) return CombatState{ 0.f, 0.f, 0.f, false };
    return e->state;
}

// ---- respawn ----------------------------------------------------------------

bool CombatSystem::Respawn(uint32_t entityId) {
    Entry* e = FindEntry(entityId);
    if (!e || e->state.alive) return false;
    e->state.health = e->state.maxHealth;
    e->state.alive  = true;
    return true;
}

// ---- lifecycle --------------------------------------------------------------

void CombatSystem::Clear() noexcept {
    m_Entities.clear();
}

} // namespace NF::Game::Gameplay
