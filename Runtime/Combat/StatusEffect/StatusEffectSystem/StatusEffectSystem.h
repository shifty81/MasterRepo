#pragma once
/**
 * @file StatusEffectSystem.h
 * @brief Buff/debuff stack: apply, tick, expire, stack, resist, query on entities.
 *
 * Features:
 *   - EffectDef: id, name, duration, tickInterval, maxStacks, statModifiers[]
 *   - RegisterEffect(def) → bool
 *   - Apply(entityId, effectId, stacks=1) → instanceId
 *   - Remove(entityId, effectId) / RemoveAll(entityId)
 *   - Tick(dt): advance timers, fire per-tick callbacks, expire effects
 *   - GetActiveEffects(entityId, outEffectIds[]) → uint32_t
 *   - GetStackCount(entityId, effectId) → uint32_t
 *   - GetRemainingTime(entityId, effectId) → float
 *   - HasEffect(entityId, effectId) → bool
 *   - SetOnApply(effectId, cb): callback(entityId, stacks)
 *   - SetOnTick(effectId, cb): callback(entityId, stacks, dt)
 *   - SetOnExpire(effectId, cb): callback(entityId)
 *   - SetResistance(entityId, effectId, chance): 0=no resist, 1=always resist
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct StatusEffectDef {
    uint32_t    id;
    std::string name;
    float       duration{5.f};
    float       tickInterval{1.f};
    uint32_t    maxStacks{1};
};

class StatusEffectSystem {
public:
    StatusEffectSystem();
    ~StatusEffectSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Registration
    bool RegisterEffect(const StatusEffectDef& def);

    // Application
    uint32_t Apply    (uint32_t entityId, uint32_t effectId, uint32_t stacks = 1);
    void     Remove   (uint32_t entityId, uint32_t effectId);
    void     RemoveAll(uint32_t entityId);

    // Per-frame
    void Tick(float dt);

    // Query
    uint32_t GetActiveEffects  (uint32_t entityId, std::vector<uint32_t>& out) const;
    uint32_t GetStackCount     (uint32_t entityId, uint32_t effectId) const;
    float    GetRemainingTime  (uint32_t entityId, uint32_t effectId) const;
    bool     HasEffect         (uint32_t entityId, uint32_t effectId) const;

    // Callbacks
    void SetOnApply (uint32_t effectId,
                     std::function<void(uint32_t entityId, uint32_t stacks)> cb);
    void SetOnTick  (uint32_t effectId,
                     std::function<void(uint32_t entityId, uint32_t stacks, float dt)> cb);
    void SetOnExpire(uint32_t effectId,
                     std::function<void(uint32_t entityId)> cb);

    // Resistance
    void SetResistance(uint32_t entityId, uint32_t effectId, float chance);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
