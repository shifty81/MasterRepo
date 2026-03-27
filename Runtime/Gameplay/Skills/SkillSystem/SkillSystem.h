#pragma once
/**
 * @file SkillSystem.h
 * @brief Player skill registry with levelling, cooldowns, resource costs, and unlock trees.
 *
 * Features:
 *   - RegisterSkill(desc): define a skill with name, max level, cost, cooldown
 *   - UnlockSkill(agentId, skillId): grant access to an agent
 *   - LevelUp(agentId, skillId): increment skill level (up to maxLevel)
 *   - Activate(agentId, skillId) → bool: trigger if unlocked, levelled, off cooldown, sufficient resource
 *   - SetResource(agentId, amount): set agent resource pool (mana/energy/stamina)
 *   - GetResource(agentId) → float
 *   - RegenerateResource(agentId, dt): apply regen rate per second
 *   - Tick(dt): advance all active cooldowns
 *   - GetCooldownRemaining(agentId, skillId) → float: seconds until usable
 *   - IsUnlocked(agentId, skillId) / GetLevel(agentId, skillId)
 *   - SetPrerequisite(skillId, prereqSkillId, prereqLevel): unlock-tree edge
 *   - CanUnlock(agentId, skillId) → bool: all prerequisites met
 *   - SetOnActivate(skillId, cb): callback on successful activation
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct SkillDesc {
    uint32_t    id{0};
    std::string name;
    uint32_t    maxLevel   {5};
    float       baseCost   {10.f}; ///< resource cost at level 1
    float       cooldownSec{1.f};
    float       costPerLevel{0.f}; ///< additional cost per level above 1
};

class SkillSystem {
public:
    SkillSystem();
    ~SkillSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();
    void Tick    (float dt);

    // Skill registration
    void     RegisterSkill  (const SkillDesc& desc);
    uint32_t GetSkillId     (const std::string& name) const;

    // Agent management
    bool UnlockSkill(uint32_t agentId, uint32_t skillId);
    bool LevelUp    (uint32_t agentId, uint32_t skillId);
    bool Activate   (uint32_t agentId, uint32_t skillId);

    // Resource
    void  SetResource        (uint32_t agentId, float amount);
    float GetResource        (uint32_t agentId) const;
    void  SetRegenRate       (uint32_t agentId, float perSec);
    void  RegenerateResource (uint32_t agentId, float dt);
    void  SetMaxResource     (uint32_t agentId, float maxAmount);

    // Queries
    bool     IsUnlocked        (uint32_t agentId, uint32_t skillId) const;
    uint32_t GetLevel          (uint32_t agentId, uint32_t skillId) const;
    float    GetCooldownRemaining(uint32_t agentId, uint32_t skillId) const;
    bool     CanUnlock         (uint32_t agentId, uint32_t skillId) const;

    // Unlock tree
    void SetPrerequisite(uint32_t skillId, uint32_t prereqSkillId, uint32_t prereqLevel=1);

    // Callbacks
    void SetOnActivate(uint32_t skillId, std::function<void(uint32_t agentId, uint32_t level)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
