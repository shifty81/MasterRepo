#pragma once
/**
 * @file GrappleSystem.h
 * @brief Grapple hook: projectile launch, rope/spring physics, swing & zip-line locomotion.
 *
 * Features:
 *   - Hook projectile: launch direction + speed, gravity, max range
 *   - Hook attach: snaps to first surface hit (via HitTestFn callback)
 *   - Rope simulation: spring-damper model between hook and owner
 *   - Swing mode: applies centripetal force constraint to owner
 *   - Zip-line mode: slides owner along rope toward hook point
 *   - Retract: reel-in at configurable speed
 *   - Detach: releases hook
 *   - Multiple simultaneous grapples per owner supported
 *   - Per-grapple state: Flying, Attached, Swinging, ZipLine, Retracting, Detached
 *   - Tick: advances projectile, applies rope forces (output force to caller)
 *   - On-attach, on-detach, on-hit callbacks
 *
 * Typical usage:
 * @code
 *   GrappleSystem gs;
 *   gs.Init();
 *   gs.SetHitTestFn([](const float from[3], const float dir[3], float maxDist,
 *                       float* outHit){ return RaycastWorld(from,dir,maxDist,outHit); });
 *   uint32_t id = gs.Launch(ownerId, firePos, fireDir, 30.f, 50.f);
 *   gs.Tick(dt);
 *   float force[3]; gs.GetRopeForce(id, force);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class GrappleState : uint8_t {
    Flying=0, Attached, Swinging, ZipLine, Retracting, Detached
};

struct GrappleDesc {
    float    launchSpeed  {30.f};
    float    maxRange     {50.f};
    float    ropeStiffness{400.f};
    float    ropeDamping  {40.f};
    float    retractSpeed {8.f};
    float    gravity      {9.8f};
    bool     autoSwing    {true};
};

struct GrappleState_ {
    uint32_t     id{0};
    uint32_t     ownerId{0};
    GrappleState state{GrappleState::Flying};
    float        hookPos[3]{};        ///< current hook world pos
    float        hookVel[3]{};        ///< flying velocity
    float        attachPoint[3]{};    ///< where hook is anchored
    float        ropeLength{0.f};     ///< current max rope length
    float        travelDist{0.f};     ///< accumulated fly distance
};

// Returns penetration depth (>0 hit), fills outHitPos
using GrappleHitFn = std::function<bool(const float from[3], const float dir[3],
                                         float maxDist, float* outHitPos)>;

class GrappleSystem {
public:
    GrappleSystem();
    ~GrappleSystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    // Launch
    uint32_t Launch(uint32_t ownerId, const float firePos[3],
                    const float fireDir[3], const GrappleDesc& desc={});

    // Control
    void Detach  (uint32_t id);
    void Retract (uint32_t id);
    void SetMode (uint32_t id, GrappleState mode);  ///< Swinging / ZipLine

    // State
    const GrappleState_* Get     (uint32_t id) const;
    GrappleState         GetState(uint32_t id) const;
    bool                 IsAlive (uint32_t id) const;

    // Force output (call after Tick to get force to apply to owner)
    void GetRopeForce(uint32_t id, const float ownerPos[3], float outForce[3]) const;

    // Hook test function
    void SetHitTestFn(GrappleHitFn fn);

    // Callbacks
    void SetOnAttach(std::function<void(uint32_t id, const float attachPos[3])> cb);
    void SetOnDetach(std::function<void(uint32_t id)> cb);

    // Query
    std::vector<uint32_t> GetByOwner(uint32_t ownerId) const;
    std::vector<uint32_t> GetAll()                      const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
