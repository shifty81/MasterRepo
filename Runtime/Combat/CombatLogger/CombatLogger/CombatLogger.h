#pragma once
/**
 * @file CombatLogger.h
 * @brief Structured combat event log: damage, heal, kill, ability use, filters, export.
 *
 * Features:
 *   - EventType: Damage, Heal, Kill, AbilityUse, StatusApply, StatusExpire, Block, Miss
 *   - LogDamage(attackerId, targetId, amount, type, isCrit)
 *   - LogHeal(casterId, targetId, amount, isCrit)
 *   - LogKill(killerId, targetId)
 *   - LogAbility(casterId, abilityId, targetId)
 *   - LogStatus(casterId, targetId, effectId, applied)
 *   - GetEventCount() → uint32_t
 *   - GetEvent(index) → CombatEvent
 *   - Filter(entityId) → vector<CombatEvent>: all events involving entity
 *   - FilterType(type) → vector<CombatEvent>
 *   - GetTotalDamageDone(entityId) → float
 *   - GetTotalDamageTaken(entityId) → float
 *   - GetTotalHealDone(entityId) → float
 *   - GetKillCount(entityId) → uint32_t
 *   - ExportCSV() → string
 *   - SetMaxEvents(n): rolling buffer cap
 *   - SetOnEvent(cb): callback per new event
 *   - Clear() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class CombatEventType : uint8_t {
    Damage, Heal, Kill, AbilityUse, StatusApply, StatusExpire, Block, Miss
};

struct CombatEvent {
    CombatEventType type;
    uint32_t        attackerId{0};
    uint32_t        targetId  {0};
    uint32_t        abilityId {0};
    uint32_t        effectId  {0};
    float           amount    {0};
    bool            isCrit    {false};
    float           timestamp {0};
};

class CombatLogger {
public:
    CombatLogger();
    ~CombatLogger();

    void Init    ();
    void Shutdown();
    void Clear   ();

    // Logging
    void LogDamage (uint32_t attacker, uint32_t target,
                    float amount, uint32_t type = 0, bool crit = false);
    void LogHeal   (uint32_t caster, uint32_t target, float amount, bool crit = false);
    void LogKill   (uint32_t killer, uint32_t target);
    void LogAbility(uint32_t caster, uint32_t abilityId, uint32_t target);
    void LogStatus (uint32_t caster, uint32_t target, uint32_t effectId, bool applied);

    // Query
    uint32_t         GetEventCount  () const;
    CombatEvent      GetEvent       (uint32_t index) const;
    std::vector<CombatEvent> Filter    (uint32_t entityId) const;
    std::vector<CombatEvent> FilterType(CombatEventType type) const;

    // Stats
    float    GetTotalDamageDone (uint32_t entityId) const;
    float    GetTotalDamageTaken(uint32_t entityId) const;
    float    GetTotalHealDone   (uint32_t entityId) const;
    uint32_t GetKillCount       (uint32_t entityId) const;

    // Export
    std::string ExportCSV() const;

    // Config
    void SetMaxEvents(uint32_t n);
    void SetOnEvent  (std::function<void(const CombatEvent&)> cb);
    void SetTimestamp(float t);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
