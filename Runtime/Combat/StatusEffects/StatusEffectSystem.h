#pragma once
/**
 * @file StatusEffectSystem.h
 * @brief Status effects (buffs/debuffs) — apply, tick, stack, and expire.
 *
 * Status effects modify entity stats or behaviour over time:
 *   - Poison: damage per second
 *   - Burn:  damage + DoT
 *   - Slow:  movement speed reduction
 *   - Stun:  disables actions
 *   - Regen: heal per second
 *   - Custom: user-defined tick / expire callbacks
 *
 * Features:
 *   - Duration, stacking (refresh vs. add stack) modes
 *   - Stat modifier application/removal
 *   - Visual / audio cue hints
 *   - Immunity tags (prevents application if entity has tag)
 *
 * Typical usage:
 * @code
 *   StatusEffectSystem ses;
 *   ses.Init();
 *   StatusEffectDef poison;
 *   poison.id        = "poison";
 *   poison.duration  = 10.f;
 *   poison.tickRate  = 1.f;
 *   poison.onTick    = [&](uint32_t eid, float dt){ DealDamage(eid, 5.f); };
 *   ses.RegisterDef(poison);
 *   ses.Apply(targetId, "poison", sourceId);
 *   ses.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class StackMode : uint8_t { Refresh=0, Additive, Max };

struct StatusEffectDef {
    std::string id;
    std::string displayName;
    float       duration{5.f};       ///< -1 = permanent
    float       tickRate{1.f};       ///< seconds between ticks (0 = no tick)
    uint32_t    maxStacks{1};
    StackMode   stackMode{StackMode::Refresh};
    std::string iconPath;
    bool        isBuff{false};
    std::vector<std::string> immunityTags;

    std::function<void(uint32_t entityId)>            onApply;
    std::function<void(uint32_t entityId, float dt)>  onTick;
    std::function<void(uint32_t entityId)>            onExpire;
    std::function<void(uint32_t entityId)>            onRemove;
};

struct ActiveEffect {
    std::string  defId;
    uint32_t     sourceId{0};
    uint32_t     stacks{1};
    float        timeRemaining{0.f};
    float        tickAccum{0.f};
};

class StatusEffectSystem {
public:
    StatusEffectSystem();
    ~StatusEffectSystem();

    void Init();
    void Shutdown();

    // Definition registry
    void RegisterDef(const StatusEffectDef& def);
    void UnregisterDef(const std::string& id);
    bool HasDef(const std::string& id) const;
    StatusEffectDef GetDef(const std::string& id) const;

    // Application
    bool Apply(uint32_t entityId, const std::string& effectId, uint32_t sourceId = 0);
    bool Remove(uint32_t entityId, const std::string& effectId);
    void RemoveAll(uint32_t entityId);
    bool HasEffect(uint32_t entityId, const std::string& effectId) const;
    uint32_t GetStacks(uint32_t entityId, const std::string& effectId) const;
    float    GetTimeRemaining(uint32_t entityId, const std::string& effectId) const;
    std::vector<ActiveEffect> GetEffects(uint32_t entityId) const;

    // Immunity
    void AddImmunity(uint32_t entityId, const std::string& tag);
    void RemoveImmunity(uint32_t entityId, const std::string& tag);
    bool IsImmune(uint32_t entityId, const std::string& effectId) const;

    void Tick(float dt);

    void OnEffectApplied(std::function<void(uint32_t entityId, const std::string& effectId)> cb);
    void OnEffectExpired(std::function<void(uint32_t entityId, const std::string& effectId)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
