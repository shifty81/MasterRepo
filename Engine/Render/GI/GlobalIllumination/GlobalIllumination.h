#pragma once
/**
 * @file GlobalIllumination.h
 * @brief Irradiance probe network for diffuse global illumination with probe placement,
 *        spherical harmonics encoding, and trilinear interpolation at query points.
 *
 * Features:
 *   - AddProbe(pos): place an irradiance probe at world position
 *   - RemoveProbe(id)
 *   - BakeProbe(id, skyColour, bounceColour): encode 9-coefficient L2 SH from 6-face cube samples
 *   - BakeAll(skyColour, bounceColour): bake every registered probe
 *   - SampleIrradiance(worldPos, normal) → RGB: trilinear blend of nearest probes projected onto normal
 *   - SetProbeCount(n): auto-place n probes on a regular grid within volume
 *   - SetVolume(min, max): define irradiance volume bounds for auto-placement
 *   - GetProbeCount() → uint32_t
 *   - GetProbePosition(id) → Vec3
 *   - SetMaxBlendProbes(k): use k nearest probes for interpolation (default 8)
 *   - InvalidateProbe(id): mark a probe dirty for re-bake
 *   - SaveToJSON(path) / LoadFromJSON(path)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Engine {

struct GIVec3 { float x, y, z; };
struct GIRgb  { float r, g, b; };

/// 9-coefficient L2 spherical harmonics (RGB channels)
struct SHCoeffs9 {
    float r[9]{}, g[9]{}, b[9]{};
};

struct IrradianceProbe {
    uint32_t  id{0};
    GIVec3    pos{};
    SHCoeffs9 sh{};
    bool      dirty{true};
};

class GlobalIllumination {
public:
    GlobalIllumination();
    ~GlobalIllumination();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Volume bounds for auto-placement
    void SetVolume(GIVec3 minBound, GIVec3 maxBound);

    // Manual probe management
    uint32_t AddProbe   (GIVec3 pos);
    void     RemoveProbe(uint32_t id);

    // Auto-placement: fills a regular grid inside the volume
    void AutoPlaceProbes(uint32_t countX, uint32_t countY, uint32_t countZ);

    // Baking (encodes simple Lambertian SH from constant sky + bounce colours)
    void BakeProbe(uint32_t id, GIRgb skyColour, GIRgb bounceColour);
    void BakeAll  (GIRgb skyColour, GIRgb bounceColour);

    // Query: trilinear blend of k nearest probes, evaluated for given normal direction
    GIRgb SampleIrradiance(GIVec3 worldPos, GIVec3 normal) const;

    // Config
    void SetMaxBlendProbes(uint32_t k);
    void InvalidateProbe  (uint32_t id);
    void InvalidateAll    ();

    // Stats
    uint32_t GetProbeCount       ()             const;
    GIVec3   GetProbePosition    (uint32_t id)  const;
    bool     IsProbeValid        (uint32_t id)  const;

    // Serialisation
    bool SaveToJSON (const std::string& path) const;
    bool LoadFromJSON(const std::string& path);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
