#pragma once
/**
 * @file AbilityQueue.h
 * @brief Ordered ability activation queue: buffering, priority, interrupt, cooldown-gate.
 *
 * Features:
 *   - RegisterAbility(id, priority, cooldown, castTime): add an ability definition
 *   - Enqueue(agentId, abilityId): push an activation request onto the queue
 *   - Tick(dt): drain queue → fire OnActivate callbacks respecting cooldowns/cast times
 *   - Interrupt(agentId): cancel current cast and flush queue for agent
 *   - IsOnCooldown(agentId, abilityId) → bool
 *   - GetCooldownRemaining(agentId, abilityId) → float
 *   - GetQueueLength(agentId) → uint32_t
 *   - SetMaxQueueLength(n): discard excess entries beyond this per-agent limit
 *   - SetOnActivate(cb): callback(agentId, abilityId) when ability fires
 *   - SetOnInterrupt(cb): callback(agentId, abilityId) when cast interrupted
 *   - SetOnCooldownEnd(cb): callback(agentId, abilityId)
 *   - ForceActivate(agentId, abilityId): bypass queue + cooldown check
 *   - ClearQueue(agentId)
 *   - SetPriorityBoost(abilityId, boost): temporarily elevate priority of an ability
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct AbilityDef {
    uint32_t id;
    int32_t  priority{0};     ///< higher = activates sooner if multiple queued
    float    cooldown{0};
    float    castTime{0};
};

class AbilityQueue {
public:
    AbilityQueue();
    ~AbilityQueue();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Ability registry
    void RegisterAbility(const AbilityDef& def);
    void SetPriorityBoost(uint32_t abilityId, int32_t boost);

    // Queue management
    void Enqueue    (uint32_t agentId, uint32_t abilityId);
    void Interrupt  (uint32_t agentId);
    void ClearQueue (uint32_t agentId);
    void ForceActivate(uint32_t agentId, uint32_t abilityId);

    // Per-frame
    void Tick(float dt);

    // Query
    bool     IsOnCooldown       (uint32_t agentId, uint32_t abilityId) const;
    float    GetCooldownRemaining(uint32_t agentId, uint32_t abilityId) const;
    uint32_t GetQueueLength     (uint32_t agentId) const;

    // Config
    void SetMaxQueueLength(uint32_t n);

    // Callbacks
    void SetOnActivate  (std::function<void(uint32_t agentId, uint32_t abilityId)> cb);
    void SetOnInterrupt (std::function<void(uint32_t agentId, uint32_t abilityId)> cb);
    void SetOnCooldownEnd(std::function<void(uint32_t agentId, uint32_t abilityId)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
