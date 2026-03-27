#pragma once
/**
 * @file ShadowSystem.h
 * @brief Shadow map generation, PCF filtering, directional/spot/point shadow casters.
 *
 * Features:
 *   - Light types: Directional, Spot, Point (cube-map)
 *   - Per-light shadow map resolution (power-of-two)
 *   - Cascaded Shadow Maps (CSM) for directional lights: up to 4 cascades
 *   - PCF (Percentage Closer Filtering): configurable kernel size 1×1..7×7
 *   - Bias settings: constant + slope-scale to combat shadow acne
 *   - Shadow distance fade: linear fade beyond MaxShadowDistance
 *   - Per-caster enable/disable + per-receiver enable/disable
 *   - Light-space matrix output (for renderer binding)
 *   - Up to 8 simultaneous shadow-casting lights
 *   - Render list: FrustumCull(light) → list of caster IDs
 *
 * Typical usage:
 * @code
 *   ShadowSystem ss;
 *   ss.Init(8, 1024);
 *   uint32_t lightId = ss.AddLight({LightType::Directional, {0,-1,0}});
 *   ss.SetCSMCascades(lightId, 3, {10.f, 50.f, 200.f});
 *   ss.SetPCF(lightId, 3);
 *   ss.Update(cameraPos, cameraDir, cameraFovDeg);
 *   const float* mat = ss.GetLightMatrix(lightId, 0);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class LightType : uint8_t { Directional=0, Spot, Point };

struct ShadowLightDesc {
    LightType type       {LightType::Directional};
    float     direction [3]{0,-1,0};   ///< Directional/Spot
    float     position  [3]{};         ///< Spot/Point
    float     spotAngle {45.f};        ///< Spot: half-angle degrees
    float     range     {100.f};       ///< Spot/Point range
    uint32_t  resolution{1024};        ///< shadow map size
    float     nearPlane {0.1f};
    float     farPlane  {500.f};
    float     biasConst {0.005f};
    float     biasSlope {0.5f};
    bool      enabled   {true};
};

struct CascadeInfo {
    float splitDistance{0.f};
    float lightMatrix[16]{};           ///< row-major 4×4
};

class ShadowSystem {
public:
    ShadowSystem();
    ~ShadowSystem();

    void Init    (uint32_t maxLights, uint32_t defaultResolution);
    void Shutdown();

    // Light management
    uint32_t AddLight  (const ShadowLightDesc& desc);
    void     RemoveLight(uint32_t lightId);
    void     UpdateLight(uint32_t lightId, const ShadowLightDesc& desc);
    bool     HasLight   (uint32_t lightId) const;

    // CSM (Directional only)
    void SetCSMCascades (uint32_t lightId, uint32_t count,
                         const std::vector<float>& splitDistances);
    uint32_t GetCascadeCount(uint32_t lightId) const;

    // PCF
    void SetPCF(uint32_t lightId, uint32_t kernelSize);  ///< 1,3,5,7

    // Distance fade
    void SetMaxShadowDistance(float d);

    // Per-frame update — recomputes light matrices from camera view
    void Update(const float camPos[3], const float camDir[3], float camFovDeg,
                float camAspect, float camNear, float camFar);

    // Output
    const float* GetLightMatrix(uint32_t lightId, uint32_t cascade=0) const;
    std::vector<uint32_t> GetActiveLights() const;

    // Caster queries
    using CasterCullFn = std::function<std::vector<uint32_t>(
        const float frustumPlanes[24], uint32_t lightId)>;
    void SetCasterCullFn(CasterCullFn fn);

    // Stats
    uint32_t ShadowCasterCount(uint32_t lightId) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
