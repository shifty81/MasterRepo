#pragma once
/**
 * @file DamageSystem.h
 * @brief Component-based damage system with typed damage, resistances, modifiers, and stacking.
 *
 * DamageSystem manages per-entity health and damage processing:
 *
 *   DamageType: Physical, Fire, Ice, Lightning, Poison, Psychic, True.
 *   DamageFlags: CanCrit, ArmorPiercing, DoT (damage-over-time), Silent.
 *
 *   DamageSource: attacker entity ID, type, base amount, flags, crit multiplier.
 *
 *   HealthComponent:
 *     - maxHp, currentHp, resistances[DamageType], flat absorption, regenRate.
 *     - isAlive(), hpFraction().
 *
 *   DamageModifier: flat bonus/penalty added before or after resistances.
 *
 *   DamageResult: final damage dealt, was it a crit, absorbed amount, was lethal.
 *
 *   DamageSystem:
 *     - RegisterEntity(id, health): attach a HealthComponent to an entity.
 *     - UnregisterEntity(id).
 *     - ApplyDamage(source, targetId) → DamageResult.
 *     - ApplyHeal(targetId, amount) → float  (actual healed).
 *     - AddModifier(targetId, modifier).
 *     - RemoveModifier(targetId, modId).
 *     - Tick(dt): process DoT stacks and HP regeneration.
 *     - GetHealth(id) → const HealthComponent*.
 *     - IsAlive(id) → bool.
 *     - SetDeathCallback(cb): fired when an entity reaches 0 HP.
 *     - SetDamageCallback(cb): fired after every damage application.
 *     - Stats(): total damage, kills, heals processed.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <array>

namespace Runtime {

// ── IDs ───────────────────────────────────────────────────────────────────
using EntityId   = uint64_t;
using ModifierId = uint32_t;
static constexpr EntityId   kInvalidEntityId   = 0;
static constexpr ModifierId kInvalidModifierId = 0;

// ── Damage type ───────────────────────────────────────────────────────────
enum class DamageType : uint8_t {
    Physical = 0,
    Fire,
    Ice,
    Lightning,
    Poison,
    Psychic,
    True,       ///< Bypasses all resistances
    _Count
};

static constexpr size_t kDamageTypeCount = static_cast<size_t>(DamageType::_Count);

// ── Damage flags ──────────────────────────────────────────────────────────
enum class DamageFlags : uint8_t {
    None          = 0,
    CanCrit       = 1 << 0,  ///< Roll for critical hit
    ArmorPiercing = 1 << 1,  ///< Ignores flat absorption
    DoT           = 1 << 2,  ///< Damage-over-time (applied via Tick)
    Silent        = 1 << 3,  ///< Does not fire damage callback
};

inline DamageFlags operator|(DamageFlags a, DamageFlags b) {
    return static_cast<DamageFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline bool HasFlag(DamageFlags flags, DamageFlags test) {
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(test)) != 0;
}

// ── Damage source ─────────────────────────────────────────────────────────
struct DamageSource {
    EntityId    attackerId{kInvalidEntityId};
    DamageType  type{DamageType::Physical};
    float       baseAmount{0.0f};
    DamageFlags flags{DamageFlags::None};
    float       critMultiplier{2.0f};  ///< Multiplier if crit occurs
    float       critChance{0.05f};     ///< [0, 1] probability of a crit
    float       dotDuration{0.0f};     ///< Seconds for DoT ticks (if DoT flag)
    float       dotInterval{1.0f};     ///< Seconds between DoT ticks
};

// ── Damage modifier ───────────────────────────────────────────────────────
enum class ModifierPhase : uint8_t {
    PreResist,   ///< Applied to base damage before resistance
    PostResist,  ///< Applied after resistance
};

struct DamageModifier {
    ModifierId   id{kInvalidModifierId};
    std::string  name;
    float        flatBonus{0.0f};      ///< Added to damage
    float        percentBonus{0.0f};   ///< Multiplied (e.g. 0.1 = +10 %)
    DamageType   affectsType{DamageType::Physical};
    bool         affectsAll{true};     ///< If true, applied to all damage types
    ModifierPhase phase{ModifierPhase::PreResist};
};

// ── DoT stack ─────────────────────────────────────────────────────────────
struct DoTStack {
    EntityId   sourceId{kInvalidEntityId};
    DamageType type{DamageType::Poison};
    float      damagePerTick{0.0f};
    float      interval{1.0f};
    float      remaining{0.0f};
    float      accumulator{0.0f};
};

// ── Health component ──────────────────────────────────────────────────────
struct HealthComponent {
    float maxHp{100.0f};
    float currentHp{100.0f};
    float regenRate{0.0f};  ///< HP/second regeneration (0 = none)
    float flatAbsorption{0.0f};  ///< Flat damage reduction before resist

    /// Resistance as a fraction [0, 1) — 0 = no resist, 0.5 = 50 % reduction.
    std::array<float, kDamageTypeCount> resistances{};

    std::vector<DamageModifier> modifiers;
    std::vector<DoTStack>       dotStacks;

    bool  isAlive()    const { return currentHp > 0.0f; }
    float hpFraction() const { return maxHp > 0.0f ? currentHp / maxHp : 0.0f; }
};

// ── Damage result ─────────────────────────────────────────────────────────
struct DamageResult {
    float   finalDamage{0.0f};
    float   absorbedAmount{0.0f};
    bool    wasCrit{false};
    bool    wasLethal{false};
    bool    targetExists{false};
};

// ── Callbacks ─────────────────────────────────────────────────────────────
using DeathCallback  = std::function<void(EntityId targetId, EntityId killerId)>;
using DamageDoneCallback = std::function<void(EntityId targetId, const DamageResult&)>;

// ── Stats ─────────────────────────────────────────────────────────────────
struct DamageSystemStats {
    uint64_t totalDamageEvents{0};
    uint64_t totalCrits{0};
    uint64_t totalKills{0};
    uint64_t totalHealEvents{0};
    float    totalDamageDealt{0.0f};
    float    totalHealingDone{0.0f};
};

// ── DamageSystem ──────────────────────────────────────────────────────────
class DamageSystem {
public:
    DamageSystem();
    ~DamageSystem();

    DamageSystem(const DamageSystem&)            = delete;
    DamageSystem& operator=(const DamageSystem&) = delete;

    // ── entity registration ────────────────────────────────────
    void RegisterEntity(EntityId id, const HealthComponent& health);
    void UnregisterEntity(EntityId id);
    bool IsRegistered(EntityId id) const;

    // ── damage & healing ───────────────────────────────────────
    DamageResult ApplyDamage(const DamageSource& source, EntityId targetId);
    float        ApplyHeal(EntityId targetId, float amount);

    // ── modifiers ──────────────────────────────────────────────
    ModifierId AddModifier(EntityId targetId, DamageModifier modifier);
    bool       RemoveModifier(EntityId targetId, ModifierId modId);
    void       ClearModifiers(EntityId targetId);

    // ── DoT management ─────────────────────────────────────────
    void AddDoT(EntityId targetId, const DoTStack& dot);
    void ClearDoTs(EntityId targetId);

    // ── simulation ─────────────────────────────────────────────
    /// Advance DoT stacks and HP regen.
    void Tick(float dt);

    // ── queries ────────────────────────────────────────────────
    const HealthComponent* GetHealth(EntityId id) const;
    bool                   IsAlive(EntityId id) const;
    float                  GetCurrentHp(EntityId id) const;
    float                  GetMaxHp(EntityId id) const;

    // ── callbacks ──────────────────────────────────────────────
    void SetDeathCallback(DeathCallback cb);
    void SetDamageCallback(DamageDoneCallback cb);

    // ── stats ──────────────────────────────────────────────────
    DamageSystemStats GetStats() const;
    void              ResetStats();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Runtime
