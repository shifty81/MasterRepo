#pragma once
/**
 * @file DamagePipeline.h
 * @brief Centralised damage processing pipeline with modifiers, armor, resistances.
 *
 * All damage in the game flows through DamagePipeline, which applies:
 *   1. Pre-damage modifiers (attacker buffs, weapon enchants)
 *   2. Armor / resistance reduction
 *   3. Critical hit roll
 *   4. Damage type conversion (e.g. fire → burn)
 *   5. Post-damage modifiers (life-steal, reflection)
 *   6. Application to target health component
 *
 * Features:
 *   - Typed damage (physical, fire, cold, lightning, poison, true)
 *   - Global and per-entity modifier stacks
 *   - Event callbacks for each stage
 *   - Hit-stun / knockback impulse generation
 *
 * Typical usage:
 * @code
 *   DamagePipeline dp;
 *   dp.Init();
 *   DamageRequest req;
 *   req.sourceId  = attackerEntityId;
 *   req.targetId  = targetEntityId;
 *   req.amount    = 25.f;
 *   req.type      = DamageType::Fire;
 *   DamageResult res = dp.Apply(req);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class DamageType : uint8_t {
    Physical=0, Fire, Cold, Lightning, Poison, Arcane, True, COUNT
};

struct DamageRequest {
    uint32_t    sourceId{0};
    uint32_t    targetId{0};
    float       amount{0.f};
    DamageType  type{DamageType::Physical};
    bool        isCritical{false};     ///< forced crit if true
    float       critMultiplier{2.f};
    float       knockbackImpulse{0.f};
    std::string sourceAbilityId;       ///< optional, for ability-specific mods
};

struct DamageResult {
    uint32_t   targetId{0};
    float      rawAmount{0.f};
    float      finalAmount{0.f};
    float      absorbed{0.f};          ///< blocked by armor/resistance
    bool       wasCritical{false};
    bool       killed{false};
    DamageType type{DamageType::Physical};
};

struct DamageModifier {
    std::string  id;
    float        flatBonus{0.f};       ///< added after everything else
    float        percentBonus{0.f};    ///< 0.2 = +20%
    DamageType   affectsType{DamageType::COUNT}; ///< COUNT = all types
    bool         isGlobal{true};
    uint32_t     entityId{0};          ///< if !isGlobal, applies to this entity
};

struct ResistanceEntry {
    uint32_t   entityId{0};
    DamageType type{DamageType::Physical};
    float      resistance{0.f};        ///< 0-1: fraction absorbed
};

class DamagePipeline {
public:
    DamagePipeline();
    ~DamagePipeline();

    void Init();
    void Shutdown();

    // Modifiers
    void AddModifier(const DamageModifier& mod);
    void RemoveModifier(const std::string& id);
    void ClearModifiers();

    // Resistances
    void SetResistance(uint32_t entityId, DamageType type, float resistance);
    float GetResistance(uint32_t entityId, DamageType type) const;
    void ClearResistances(uint32_t entityId);

    // Processing
    DamageResult Apply(const DamageRequest& req);
    DamageResult Preview(const DamageRequest& req) const; ///< no side effects

    // Health tracking (simplified: external health component registers here)
    void  RegisterEntity(uint32_t id, float maxHp);
    void  UnregisterEntity(uint32_t id);
    bool  IsAlive(uint32_t id) const;
    float GetHealth(uint32_t id) const;
    float GetMaxHealth(uint32_t id) const;
    void  SetHealth(uint32_t id, float hp);
    void  Heal(uint32_t id, float amount);

    // Callbacks
    void OnDamageApplied(std::function<void(const DamageResult&)> cb);
    void OnEntityDied(std::function<void(uint32_t entityId, uint32_t killerId)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
