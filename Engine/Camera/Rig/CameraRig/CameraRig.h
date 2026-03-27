#pragma once
/**
 * @file CameraRig.h
 * @brief Multi-target tracking camera with spring offset, look-ahead, and obstacle avoidance.
 *
 * Features:
 *   - SetTarget(pos, velocity): primary target position and velocity for look-ahead
 *   - AddSecondaryTarget(id, pos, weight): blend multiple targets with weights
 *   - RemoveSecondaryTarget(id)
 *   - SetOffset(offset): ideal camera offset from the weighted target centre
 *   - SetSpring(stiffness, damping): spring constants for smooth follow
 *   - SetLookAheadFactor(k): amount to extrapolate target velocity
 *   - Update(dt) → CameraRigState: new camera position + lookAt point
 *   - GetPosition() / GetLookAt() → Vec3
 *   - SetAvoidRadius(r): collision avoidance sphere radius (contracts spring)
 *   - SetOnObstacleQuery(fn): callback(pos, dir, len) → hit distance; caller does ray/sphere
 *   - SetPitchLimits(minDeg, maxDeg): clamp vertical angle
 *   - SetDistanceLimits(minDist, maxDist): clamp spring length
 *   - SetFOV(deg) / GetFOV() → float
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct CRVec3 { float x, y, z; };

struct CameraRigState {
    CRVec3 pos;
    CRVec3 lookAt;
    float  fovDeg{60.f};
};

class CameraRig {
public:
    CameraRig();
    ~CameraRig();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Primary target
    void SetTarget(CRVec3 pos, CRVec3 velocity={0,0,0});

    // Secondary targets
    void AddSecondaryTarget   (uint32_t id, CRVec3 pos, float weight=1.f);
    void RemoveSecondaryTarget(uint32_t id);
    void UpdateSecondaryTarget(uint32_t id, CRVec3 pos);

    // Rig config
    void SetOffset         (CRVec3 offset);
    void SetSpring         (float stiffness, float damping);
    void SetLookAheadFactor(float k);
    void SetAvoidRadius    (float r);
    void SetPitchLimits    (float minDeg, float maxDeg);
    void SetDistanceLimits (float minDist, float maxDist);
    void SetFOV            (float deg);

    // Obstacle callback: returns distance to nearest hit along ray (FLT_MAX = none)
    void SetOnObstacleQuery(std::function<float(CRVec3 pos, CRVec3 dir, float len)> fn);

    // Per-frame update
    CameraRigState Update(float dt);

    // Query
    CRVec3 GetPosition() const;
    CRVec3 GetLookAt  () const;
    float  GetFOV     () const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
