#pragma once
/**
 * @file ParticleAttractor.h
 * @brief World-space attractor fields (point, ring, vortex) applied to live particle systems.
 *
 * Features:
 *   - Attractor types: Point, Ring, Vortex, Plane
 *   - CreateAttractor(type, pos, params) → attractorId
 *   - RemoveAttractor(id)
 *   - SetAttractorPos(id, pos) / SetAttractorEnabled(id, enabled)
 *   - ApplyToParticles(positions[], velocities[], masses[], count, dt):
 *       computes and adds per-particle velocity deltas from all active attractors
 *   - Point: radial pull/push proportional to 1/r² (clamped)
 *   - Ring: torus-shaped pull toward ring torus surface
 *   - Vortex: tangential rotation + axial pull
 *   - Plane: one-sided half-space pull
 *   - SetStrength(id, s): signed force magnitude
 *   - SetRadius(id, r): influence radius (beyond which force = 0)
 *   - SetFalloff(id, exponent): distance falloff exponent
 *   - GetAttractorCount() → uint32_t
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <vector>

namespace Engine {

enum class AttractorType : uint8_t { Point, Ring, Vortex, Plane };

struct PAVec3 { float x, y, z; };

struct AttractorParams {
    float strength {10.f};   ///< force magnitude (negative = repel)
    float radius   {5.f};    ///< influence radius
    float falloff  {2.f};    ///< distance exponent
    PAVec3 axis    {0,1,0};  ///< axis for Vortex/Ring/Plane
    float ringRadius{2.f};   ///< torus major radius (Ring only)
};

class ParticleAttractor {
public:
    ParticleAttractor();
    ~ParticleAttractor();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Attractor lifecycle
    uint32_t CreateAttractor (AttractorType type, PAVec3 pos, const AttractorParams& p={});
    void     RemoveAttractor (uint32_t id);

    // Per-frame control
    void SetAttractorPos    (uint32_t id, PAVec3 pos);
    void SetAttractorEnabled(uint32_t id, bool enabled);
    void SetStrength        (uint32_t id, float s);
    void SetRadius          (uint32_t id, float r);
    void SetFalloff         (uint32_t id, float exponent);

    // Apply to a particle batch (SoA layout)
    // positions:  float[count*3], velocities: float[count*3], masses: float[count]
    void ApplyToParticles(float* positions, float* velocities,
                          const float* masses, uint32_t count, float dt) const;

    uint32_t GetAttractorCount() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
