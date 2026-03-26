#pragma once
/**
 * @file AbilitySystem.h
 * @brief Gameplay Ability System (GAS) — grant, activate, cool-down, and cancel abilities.
 *
 * Abilities are named gameplay actions that may:
 *   - Have resource costs (mana, stamina, charges)
 *   - Have cooldowns
 *   - Apply status effects on hit
 *   - Be activated by input, AI, or trigger
 *   - Be blocked/cancelled by tags
 *
 * Typical usage:
 * @code
 *   AbilitySystem as;
 *   as.Init();
 *   AbilityDesc d;
 *   d.id       = "fireball";
 *   d.manaCost = 30.f;
 *   d.cooldown = 3.f;
 *   d.onActivate = [](AbilityContext& ctx){ SpawnFireball(ctx.casterId); };
 *   as.Grant(playerId, d);
 *   as.Activate("fireball", playerId);
 *   as.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class AbilityState : uint8_t { Ready=0, Active, Cooldown, Blocked };

struct AbilityContext {
    uint32_t    casterId{0};
    uint32_t    targetId{0};
    float       targetPos[3]{};
    float       castTime{0.f};
};

struct AbilityDesc {
    std::string id;
    std::string displayName;
    float       manaCost{0.f};
    float       staminaCost{0.f};
    float       cooldown{0.f};
    float       castTime{0.f};           ///< 0 = instant
    float       duration{0.f};           ///< 0 = instantaneous effect
    uint32_t    charges{1};
    std::vector<std::string> blockedByTags;  ///< tags that prevent activation
    std::vector<std::string> cancelledByTags;
    std::function<void(AbilityContext&)> onActivate;
    std::function<void(AbilityContext&)> onEnd;
    std::function<void(AbilityContext&)> onInterrupt;
};

struct AbilityInstance {
    uint32_t    ownerId{0};
    AbilityDesc desc;
    AbilityState state{AbilityState::Ready};
    float       cooldownRemaining{0.f};
    float       activeTime{0.f};
    uint32_t    chargesRemaining{1};
};

struct EntityResources {
    float mana{100.f};
    float maxMana{100.f};
    float manaRegen{5.f};        ///< per second
    float stamina{100.f};
    float maxStamina{100.f};
    float staminaRegen{10.f};
};

class AbilitySystem {
public:
    AbilitySystem();
    ~AbilitySystem();

    void Init();
    void Shutdown();

    // Entity resources
    void RegisterEntity(uint32_t id, const EntityResources& res = {});
    void UnregisterEntity(uint32_t id);
    EntityResources GetResources(uint32_t id) const;
    void SetResources(uint32_t id, const EntityResources& res);

    // Ability granting
    void Grant(uint32_t entityId, const AbilityDesc& desc);
    void Revoke(uint32_t entityId, const std::string& abilityId);
    bool HasAbility(uint32_t entityId, const std::string& abilityId) const;
    AbilityInstance GetAbility(uint32_t entityId, const std::string& abilityId) const;
    std::vector<AbilityInstance> GetAbilities(uint32_t entityId) const;

    // Activation
    bool     CanActivate(const std::string& abilityId, uint32_t entityId) const;
    bool     Activate(const std::string& abilityId, uint32_t entityId,
                      uint32_t targetId = 0, const float* targetPos = nullptr);
    void     Cancel(const std::string& abilityId, uint32_t entityId);
    void     CancelAll(uint32_t entityId);
    AbilityState GetState(const std::string& abilityId, uint32_t entityId) const;

    // Tags (for blocking/cancellation)
    void AddTag(uint32_t entityId, const std::string& tag);
    void RemoveTag(uint32_t entityId, const std::string& tag);
    bool HasTag(uint32_t entityId, const std::string& tag) const;

    void Tick(float dt);

    void OnActivated(std::function<void(uint32_t entityId, const std::string& abilityId)> cb);
    void OnEnded(std::function<void(uint32_t entityId, const std::string& abilityId)> cb);
    void OnFailed(std::function<void(uint32_t entityId, const std::string& abilityId,
                                     const std::string& reason)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
