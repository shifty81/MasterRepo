#pragma once
/**
 * @file TrapSystem.h
 * @brief Trap entity placement, trigger zones, activation logic, cooldown.
 *
 * Features:
 *   - Trap types: Spike, Arrow, Explosion, Pit, Gas (extensible via callback)
 *   - Trigger shapes: AABB or Sphere proximity
 *   - Trigger conditions: OnEnter, OnStay, OnProximity, OnTimer
 *   - Activation: one-shot or repeatable with cooldown
 *   - Pre-activation delay
 *   - Arm/Disarm state; disarmed traps don't trigger
 *   - On-activate callback: delivers TrapEvent (type, position, damage)
 *   - Visibility: hidden traps can be detected (DetectionChance)
 *   - Trap owner/team (friendly-fire flag)
 *   - Tick-based: Tick(dt) advances timers, checks trigger zones
 *
 * Typical usage:
 * @code
 *   TrapSystem ts;
 *   ts.Init();
 *   TrapDesc d; d.type=TrapType::Spike; d.position={x,y,z};
 *   d.damage=50.f; d.cooldown=3.f; d.triggerRadius=1.5f;
 *   uint32_t id = ts.Place(d);
 *   ts.SetOnActivate([](const TrapEvent& e){ ApplyDamage(e); });
 *   ts.TriggerCheck(id, actorPos);
 *   ts.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class TrapType : uint8_t { Spike=0, Arrow, Explosion, Pit, Gas, Custom };
enum class TriggerMode : uint8_t { OnEnter=0, OnStay, Proximity, Timer };

struct TrapDesc {
    TrapType    type           {TrapType::Spike};
    TriggerMode trigger        {TriggerMode::OnEnter};
    float       position       [3]{};
    float       triggerRadius  {1.f};           ///< Sphere trigger
    float       triggerHalf    [3]{0.5f,0.5f,0.5f}; ///< AABB half-ext
    bool        useAABB        {false};
    float       damage         {25.f};
    float       cooldown       {5.f};
    float       preDelay       {0.f};
    bool        oneShot        {true};
    bool        hidden         {false};
    float       detectionChance{0.5f};
    uint32_t    ownerTeam      {0};
    bool        friendlyFire   {false};
    std::string customType;
};

struct TrapEvent {
    uint32_t    trapId{0};
    TrapType    type{TrapType::Spike};
    float       position[3]{};
    float       damage{0.f};
    uint32_t    targetId{0};
};

class TrapSystem {
public:
    TrapSystem();
    ~TrapSystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    // Trap management
    uint32_t Place  (const TrapDesc& desc);
    void     Remove (uint32_t id);
    bool     Has    (uint32_t id) const;

    // State
    void Arm    (uint32_t id);
    void Disarm (uint32_t id);
    bool IsArmed(uint32_t id) const;
    bool IsCoolingDown(uint32_t id) const;
    float CooldownRemaining(uint32_t id) const;

    // Detection
    bool DetectTrap(uint32_t id, float detectorSkill=1.f) const;
    void Reveal    (uint32_t id);

    // Manual trigger
    void TriggerCheck(uint32_t id, const float actorPos[3], uint32_t actorId=0);
    void ForceActivate(uint32_t id, uint32_t targetId=0);

    // Callbacks
    using ActivateCb = std::function<void(const TrapEvent&)>;
    void SetOnActivate(ActivateCb cb);
    void SetOnDetected(std::function<void(uint32_t trapId)> cb);

    // Queries
    std::vector<uint32_t> GetAll()           const;
    std::vector<uint32_t> GetArmed()         const;
    std::vector<uint32_t> GetNearby(const float pos[3], float radius) const;
    const TrapDesc*       GetDesc(uint32_t id) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
