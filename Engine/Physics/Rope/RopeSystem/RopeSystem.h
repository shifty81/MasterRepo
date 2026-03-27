#pragma once
/**
 * @file RopeSystem.h
 * @brief Verlet rope chain: distance constraints, gravity, pin endpoints, wind, collision.
 *
 * Features:
 *   - RopeDesc: segmentCount, segmentLength, mass, stiffness, damping, gravity
 *   - Verlet integration per segment
 *   - Distance constraints (solver iterations configurable)
 *   - PinSegment(index, worldPos) / UnpinSegment(index)
 *   - Wind force: constant + turbulence amplitude, turbulence frequency
 *   - Sphere obstacle collision
 *   - Plane collision (ground)
 *   - Multiple ropes
 *   - GetSegmentPos(ropeId, segIdx) → float[3]
 *   - GetTangent(ropeId, segIdx) → float[3]
 *   - AddImpulse per segment
 *   - On-break callback (when tension exceeds breakThreshold)
 *
 * Typical usage:
 * @code
 *   RopeSystem rs;
 *   rs.Init();
 *   RopeDesc d; d.segmentCount=20; d.segmentLength=0.2f;
 *   uint32_t id = rs.Create(d, {0,5,0});
 *   rs.Pin(id, 0, {0,5,0});
 *   rs.SetWind(id, {1,0,0}, 0.5f, 2.f);
 *   rs.Tick(dt);
 *   const float* tip = rs.GetSegmentPos(id, 19);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct RopeDesc {
    uint32_t segmentCount  {20};
    float    segmentLength {0.2f};
    float    mass          {0.05f};
    float    stiffness     {0.9f};     ///< constraint correction factor [0,1]
    float    damping       {0.02f};
    float    gravity[3]    {0,-9.81f,0};
    uint32_t solverIter    {8};
    float    breakThreshold{-1.f};     ///< <0 = no break
};

struct RopeSphere { float centre[3]{}; float radius{0.5f}; };

class RopeSystem {
public:
    RopeSystem();
    ~RopeSystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    uint32_t Create (const RopeDesc& desc, const float startPos[3]=nullptr);
    void     Destroy(uint32_t id);
    bool     Has    (uint32_t id) const;

    // Pins
    void Pin  (uint32_t id, uint32_t segIdx, const float worldPos[3]);
    void Unpin(uint32_t id, uint32_t segIdx);

    // Forces
    void SetWind     (uint32_t id, const float dir[3], float turbAmp, float turbFreq);
    void AddImpulse  (uint32_t id, uint32_t segIdx, const float impulse[3]);

    // Colliders
    void AddSphere   (uint32_t id, const RopeSphere& s);
    void AddPlane    (uint32_t id, float normalY, float d); ///< y-plane shortcut
    void ClearColliders(uint32_t id);

    // Query
    const float* GetSegmentPos(uint32_t id, uint32_t idx) const;
    const float* GetTangent   (uint32_t id, uint32_t idx) const;
    uint32_t     SegmentCount (uint32_t id) const;
    float        GetTension   (uint32_t id, uint32_t segIdx) const;

    void SetOnBreak(std::function<void(uint32_t id)> cb);

    std::vector<uint32_t> GetAll() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
