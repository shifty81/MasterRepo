#include "Runtime/Damage/DamageSystem/DamageSystem.h"
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

namespace Runtime {

// ── Impl ──────────────────────────────────────────────────────────────────
struct DamageSystem::Impl {
    std::unordered_map<EntityId, HealthComponent> entities;
    ModifierId                nextModId{1};
    DamageSystemStats         stats;
    DeathCallback             deathCb;
    DamageDoneCallback        damageCb;

    std::default_random_engine rng{42};

    HealthComponent* Find(EntityId id) {
        auto it = entities.find(id);
        return it != entities.end() ? &it->second : nullptr;
    }
    const HealthComponent* Find(EntityId id) const {
        auto it = entities.find(id);
        return it != entities.end() ? &it->second : nullptr;
    }

    float ApplyModifiers(const std::vector<DamageModifier>& mods,
                         DamageType type, float damage,
                         ModifierPhase phase) const {
        for (const auto& m : mods) {
            if (m.phase != phase) continue;
            if (!m.affectsAll && m.affectsType != type) continue;
            damage += m.flatBonus;
            damage *= (1.0f + m.percentBonus);
        }
        return damage;
    }
};

// ── Constructor / Destructor ──────────────────────────────────────────────
DamageSystem::DamageSystem()  : m_impl(std::make_unique<Impl>()) {}
DamageSystem::~DamageSystem() = default;

// ── Entity registration ───────────────────────────────────────────────────
void DamageSystem::RegisterEntity(EntityId id, const HealthComponent& health) {
    m_impl->entities[id] = health;
}
void DamageSystem::UnregisterEntity(EntityId id) {
    m_impl->entities.erase(id);
}
bool DamageSystem::IsRegistered(EntityId id) const {
    return m_impl->entities.count(id) != 0;
}

// ── Damage application ────────────────────────────────────────────────────
DamageResult DamageSystem::ApplyDamage(const DamageSource& src, EntityId targetId) {
    DamageResult result;
    auto* hp = m_impl->Find(targetId);
    if (!hp || !hp->isAlive()) return result;
    result.targetExists = true;

    // Handle DoT: push a stack, do not apply immediate damage
    if (HasFlag(src.flags, DamageFlags::DoT)) {
        DoTStack dot;
        dot.sourceId       = src.attackerId;
        dot.type           = src.type;
        dot.damagePerTick  = src.baseAmount;
        dot.interval       = src.dotInterval > 0.0f ? src.dotInterval : 1.0f;
        dot.remaining      = src.dotDuration;
        hp->dotStacks.push_back(dot);
        return result;
    }

    float damage = src.baseAmount;

    // Pre-resist modifiers
    damage = m_impl->ApplyModifiers(hp->modifiers, src.type, damage,
                                    ModifierPhase::PreResist);

    // Critical hit
    if (HasFlag(src.flags, DamageFlags::CanCrit)) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        if (dist(m_impl->rng) < src.critChance) {
            damage *= src.critMultiplier;
            result.wasCrit = true;
            ++m_impl->stats.totalCrits;
        }
    }

    // Flat absorption (unless armor-piercing)
    if (!HasFlag(src.flags, DamageFlags::ArmorPiercing)) {
        float absorbed = std::min(damage, hp->flatAbsorption);
        result.absorbedAmount = absorbed;
        damage -= absorbed;
    }

    // Resistance (True damage bypasses)
    if (src.type != DamageType::True) {
        size_t idx = static_cast<size_t>(src.type);
        if (idx < kDamageTypeCount) {
            float resist = std::clamp(hp->resistances[idx], 0.0f, 0.99f);
            damage *= (1.0f - resist);
        }
    }

    // Post-resist modifiers
    damage = m_impl->ApplyModifiers(hp->modifiers, src.type, damage,
                                    ModifierPhase::PostResist);

    damage = std::max(0.0f, damage);
    hp->currentHp -= damage;
    result.finalDamage = damage;

    ++m_impl->stats.totalDamageEvents;
    m_impl->stats.totalDamageDealt += damage;

    if (hp->currentHp <= 0.0f) {
        hp->currentHp = 0.0f;
        result.wasLethal = true;
        ++m_impl->stats.totalKills;
        if (m_impl->deathCb) m_impl->deathCb(targetId, src.attackerId);
    }

    if (!HasFlag(src.flags, DamageFlags::Silent) && m_impl->damageCb) {
        m_impl->damageCb(targetId, result);
    }

    return result;
}

float DamageSystem::ApplyHeal(EntityId targetId, float amount) {
    auto* hp = m_impl->Find(targetId);
    if (!hp || !hp->isAlive()) return 0.0f;
    float actual = std::min(amount, hp->maxHp - hp->currentHp);
    hp->currentHp += actual;
    ++m_impl->stats.totalHealEvents;
    m_impl->stats.totalHealingDone += actual;
    return actual;
}

// ── Modifiers ─────────────────────────────────────────────────────────────
ModifierId DamageSystem::AddModifier(EntityId targetId, DamageModifier modifier) {
    auto* hp = m_impl->Find(targetId);
    if (!hp) return kInvalidModifierId;
    modifier.id = m_impl->nextModId++;
    hp->modifiers.push_back(std::move(modifier));
    return hp->modifiers.back().id;
}

bool DamageSystem::RemoveModifier(EntityId targetId, ModifierId modId) {
    auto* hp = m_impl->Find(targetId);
    if (!hp) return false;
    auto& mods = hp->modifiers;
    auto it = std::remove_if(mods.begin(), mods.end(),
        [modId](const DamageModifier& m){ return m.id == modId; });
    if (it == mods.end()) return false;
    mods.erase(it, mods.end());
    return true;
}

void DamageSystem::ClearModifiers(EntityId targetId) {
    if (auto* hp = m_impl->Find(targetId)) hp->modifiers.clear();
}

// ── DoT management ────────────────────────────────────────────────────────
void DamageSystem::AddDoT(EntityId targetId, const DoTStack& dot) {
    if (auto* hp = m_impl->Find(targetId)) hp->dotStacks.push_back(dot);
}

void DamageSystem::ClearDoTs(EntityId targetId) {
    if (auto* hp = m_impl->Find(targetId)) hp->dotStacks.clear();
}

// ── Tick ──────────────────────────────────────────────────────────────────
void DamageSystem::Tick(float dt) {
    for (auto& [id, hp] : m_impl->entities) {
        if (!hp.isAlive()) continue;

        // HP regen
        if (hp.regenRate > 0.0f) {
            hp.currentHp = std::min(hp.maxHp, hp.currentHp + hp.regenRate * dt);
        }

        // DoT ticks
        auto& stacks = hp.dotStacks;
        for (auto& dot : stacks) {
            dot.remaining   -= dt;
            dot.accumulator += dt;
            while (dot.accumulator >= dot.interval) {
                dot.accumulator -= dot.interval;
                DamageSource src;
                src.attackerId = dot.sourceId;
                src.type       = dot.type;
                src.baseAmount = dot.damagePerTick;
                src.flags      = DamageFlags::Silent;
                ApplyDamage(src, id);
            }
        }
        // Expire finished DoTs
        stacks.erase(std::remove_if(stacks.begin(), stacks.end(),
            [](const DoTStack& d){ return d.remaining <= 0.0f; }), stacks.end());
    }
}

// ── Queries ───────────────────────────────────────────────────────────────
const HealthComponent* DamageSystem::GetHealth(EntityId id) const {
    return m_impl->Find(id);
}
bool  DamageSystem::IsAlive(EntityId id) const {
    const auto* hp = m_impl->Find(id);
    return hp && hp->isAlive();
}
float DamageSystem::GetCurrentHp(EntityId id) const {
    const auto* hp = m_impl->Find(id);
    return hp ? hp->currentHp : 0.0f;
}
float DamageSystem::GetMaxHp(EntityId id) const {
    const auto* hp = m_impl->Find(id);
    return hp ? hp->maxHp : 0.0f;
}

// ── Callbacks ─────────────────────────────────────────────────────────────
void DamageSystem::SetDeathCallback(DeathCallback cb)       { m_impl->deathCb  = std::move(cb); }
void DamageSystem::SetDamageCallback(DamageDoneCallback cb) { m_impl->damageCb = std::move(cb); }

// ── Stats ─────────────────────────────────────────────────────────────────
DamageSystemStats DamageSystem::GetStats() const { return m_impl->stats; }
void              DamageSystem::ResetStats()     { m_impl->stats = {}; }

} // namespace Runtime
