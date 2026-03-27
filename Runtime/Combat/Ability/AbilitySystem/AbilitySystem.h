#pragma once
/**
 * @file AbilitySystem.h
 * @brief Grant, activate, cooldown, cost, modify and chain abilities on entities.
 *
 * Features:
 *   - AbilityDef: id, name, cooldown, cost (resource/type), castTime, charges
 *   - RegisterAbility(def) → bool
 *   - GrantAbility(entityId, abilityId) / RevokeAbility(entityId, abilityId)
 *   - ActivateAbility(entityId, abilityId) → bool: checks cooldown, cost, cast
 *   - CancelAbility(entityId, abilityId)
 *   - Tick(dt): advance cooldowns and cast times
 *   - IsOnCooldown(entityId, abilityId) → bool
 *   - GetCooldownRemaining(entityId, abilityId) → float
 *   - GetChargesRemaining(entityId, abilityId) → uint32_t
 *   - GetGrantedAbilities(entityId, out[]) → uint32_t
 *   - HasAbility(entityId, abilityId) → bool
 *   - AddCooldownModifier(entityId, abilityId, mult): e.g. 0.5 = half cooldown
 *   - SetResource(entityId, resourceType, amount): mana/energy/rage
 *   - GetResource(entityId, resourceType) → float
 *   - SetOnActivate(abilityId, cb) / SetOnComplete(abilityId, cb) / SetOnCancelled(cb)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct AbilityDef {
    uint32_t    id;
    std::string name;
    float       cooldown    {1.f};
    float       castTime    {0.f};
    uint32_t    resourceType{0};
    float       cost        {0.f};
    uint32_t    maxCharges  {1};
};

class AbilitySystem {
public:
    AbilitySystem();
    ~AbilitySystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Registration
    bool RegisterAbility(const AbilityDef& def);

    // Granting
    void GrantAbility (uint32_t entityId, uint32_t abilityId);
    void RevokeAbility(uint32_t entityId, uint32_t abilityId);
    bool HasAbility   (uint32_t entityId, uint32_t abilityId) const;

    // Activation
    bool ActivateAbility(uint32_t entityId, uint32_t abilityId);
    void CancelAbility  (uint32_t entityId, uint32_t abilityId);

    // Per-frame
    void Tick(float dt);

    // Query
    bool     IsOnCooldown       (uint32_t entityId, uint32_t abilityId) const;
    float    GetCooldownRemaining(uint32_t entityId, uint32_t abilityId) const;
    uint32_t GetChargesRemaining (uint32_t entityId, uint32_t abilityId) const;
    uint32_t GetGrantedAbilities (uint32_t entityId, std::vector<uint32_t>& out) const;

    // Modifiers
    void AddCooldownModifier(uint32_t entityId, uint32_t abilityId, float mult);

    // Resources
    void  SetResource(uint32_t entityId, uint32_t resourceType, float amount);
    float GetResource(uint32_t entityId, uint32_t resourceType) const;

    // Callbacks
    void SetOnActivate  (uint32_t abilityId,
                         std::function<void(uint32_t entityId)> cb);
    void SetOnComplete  (uint32_t abilityId,
                         std::function<void(uint32_t entityId)> cb);
    void SetOnCancelled (uint32_t abilityId,
                         std::function<void(uint32_t entityId)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
