#pragma once
/**
 * @file AimAssist.h
 * @brief Controller aim-assist: target magnetism, bullet-bend, sticky aim, and deadzone.
 *
 * Features:
 *   - RegisterTarget(id, worldPos, radius): add aimable target with hitbox radius
 *   - UnregisterTarget(id)
 *   - UpdateTargetPos(id, worldPos): refresh position each frame
 *   - ComputeAssist(aimDir, cameraPos, fovY): returns adjusted aim direction
 *     - Magnetism: smoothly bends aim direction toward nearest target in FoV cone
 *     - Sticky aim: reduces stick input speed when near target centre
 *   - SetMagnetismStrength(s)  [0,1]: how strongly aim snaps toward target
 *   - SetStickyRadius(r): angular radius (radians) for sticky-aim slowdown
 *   - SetStickyStrength(s) [0,1]: input attenuation in sticky zone
 *   - SetAssistFoV(radians): half-angle of assist cone (default 15°)
 *   - SetMaxDistance(d): ignore targets beyond this distance
 *   - GetLockedTarget() → targetId or 0
 *   - Reset()
 */

#include <cstdint>
#include <unordered_map>

namespace Engine {

struct AimVec3 { float x, y, z; };

class AimAssist {
public:
    AimAssist();
    ~AimAssist();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Target management
    void RegisterTarget   (uint32_t id, AimVec3 worldPos, float radius=0.5f);
    void UnregisterTarget (uint32_t id);
    void UpdateTargetPos  (uint32_t id, AimVec3 worldPos);

    // Per-frame assist computation
    // aimDir:     normalised world-space view direction
    // cameraPos:  world-space camera origin
    // Returns:    adjusted (magnetised) aim direction (normalised)
    AimVec3 ComputeAssist(AimVec3 aimDir, AimVec3 cameraPos) const;

    // Sticky input attenuation factor (call once per frame after ComputeAssist)
    // Returns [0,1] multiplier to apply to raw stick delta
    float   StickyFactor (AimVec3 aimDir, AimVec3 cameraPos) const;

    // Config
    void SetMagnetismStrength(float s); ///< [0,1]
    void SetStickyRadius     (float r); ///< radians
    void SetStickyStrength   (float s); ///< [0,1]
    void SetAssistFoV        (float r); ///< half-angle radians, default 0.26 (~15°)
    void SetMaxDistance      (float d);

    uint32_t GetLockedTarget() const; ///< id of current best target, 0 if none

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
