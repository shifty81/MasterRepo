#pragma once
/**
 * @file MagnetSystem.h
 * @brief Runtime magnet fields: attract/repel, radius, falloff, multi-pole, per-body override.
 *
 * Features:
 *   - MagnetDesc: position[3], strength (+ attract / - repel), radius, falloff exponent, polarity
 *   - Create(desc) → magnetId
 *   - RegisterBody(bodyId, mass, position[3])
 *   - Tick(dt): apply accumulated force to each registered body
 *   - GetForce(bodyId) → float[3] (accumulated for last Tick)
 *   - SetPolarity(magnetId, float) — runtime strength change / flip
 *   - OverrideBodyResponse(bodyId, magnetId, scale) — body-specific response factor
 *   - Multiple magnets, multiple bodies
 *   - GetMagnetPos / MoveMAGNET(magnetId, pos)
 *   - IsBodyAffected(bodyId, magnetId) → bool (within radius)
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct MagnetDesc {
    float    pos[3]      {0,0,0};
    float    strength    {10.f};   ///< >0 attract, <0 repel
    float    radius      {5.f};
    float    falloffExp  {2.f};    ///< distance^falloffExp
    bool     enabled     {true};
};

class MagnetSystem {
public:
    MagnetSystem();
    ~MagnetSystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    // Magnets
    uint32_t Create (const MagnetDesc& desc);
    void     Destroy(uint32_t id);
    bool     Has    (uint32_t id) const;
    void     SetStrength (uint32_t id, float s);
    void     SetEnabled  (uint32_t id, bool e);
    void     MoveMAGNET  (uint32_t id, const float pos[3]);
    void     GetMagnetPos(uint32_t id, float out[3]) const;

    // Bodies
    void RegisterBody  (uint32_t bodyId, float mass, const float pos[3]);
    void UnregisterBody(uint32_t bodyId);
    void UpdateBodyPos (uint32_t bodyId, const float pos[3]);
    const float* GetForce(uint32_t bodyId) const; ///< float[3]

    // Per-body response override
    void SetBodyMagnetScale(uint32_t bodyId, uint32_t magnetId, float scale);

    // Query
    bool IsBodyAffected(uint32_t bodyId, uint32_t magnetId) const;
    std::vector<uint32_t> GetAllMagnets() const;
    std::vector<uint32_t> GetAllBodies () const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
