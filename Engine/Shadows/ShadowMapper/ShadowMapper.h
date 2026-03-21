#pragma once
/**
 * @file ShadowMapper.h
 * @brief Cascaded shadow map (CSM) configuration and light-space matrix computation.
 *
 * ShadowMapper manages the data needed to render directional shadow maps:
 *   - CascadeConfig: per-cascade split distance, resolution, depth bias, normal bias.
 *   - LightSpaceMatrix: view+projection matrix for one cascade.
 *   - ShadowMapper::ComputeCascades(viewProj, lightDir, near, far): splits the view
 *     frustum, computes a tight orthographic projection for each cascade, and returns
 *     the LightSpaceMatrix array.
 *   - PCFConfig: kernel size, sample count, radius for soft-shadow filtering.
 *   - ShadowMap: opaque handle (maps to GPU texture slot in a real renderer).
 *
 * This module is renderer-agnostic; it produces matrices and config structs that
 * a rendering backend (OpenGL, Vulkan, etc.) can consume directly.
 */

#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include "Engine/Math/Math.h"

namespace Engine {

// ── Cascade config ────────────────────────────────────────────────────────
struct CascadeConfig {
    uint32_t resolution{1024};   ///< Shadow map texture resolution (square)
    float    splitLambda{0.75f}; ///< Practical split scheme blend [0=uniform,1=log]
    float    depthBias{0.005f};
    float    normalBias{0.02f};
    float    lightBleedReduction{0.2f};
};

// ── PCF config ────────────────────────────────────────────────────────────
struct PCFConfig {
    uint32_t kernelSize{3};     ///< NxN Poisson kernel (1=hard shadows)
    uint32_t samples{16};       ///< Sample count for random PCF
    float    radius{1.5f};      ///< Search radius in texels
};

// ── Light-space matrix for one cascade ────────────────────────────────────
struct LightSpaceMatrix {
    Math::Mat4 view;
    Math::Mat4 proj;
    Math::Mat4 viewProj;
    float      splitNear{0};
    float      splitFar{0};
    uint32_t   cascadeIndex{0};
};

// ── Shadow map handle ──────────────────────────────────────────────────────
struct ShadowMap {
    uint32_t    lightId{0};
    uint32_t    textureHandle{0};   ///< GPU texture ID (0 = not allocated)
    uint32_t    resolution{0};
    std::vector<LightSpaceMatrix> cascades;
    bool        valid{false};
};

// ── Mapper ────────────────────────────────────────────────────────────────
class ShadowMapper {
public:
    ShadowMapper();
    explicit ShadowMapper(uint32_t numCascades, const CascadeConfig& cfg = {});
    ~ShadowMapper();

    // ── configuration ─────────────────────────────────────────
    void SetCascadeCount(uint32_t n);
    uint32_t CascadeCount() const;
    void SetCascadeConfig(const CascadeConfig& cfg);
    const CascadeConfig& GetCascadeConfig() const;
    void SetPCFConfig(const PCFConfig& cfg);
    const PCFConfig& GetPCFConfig() const;

    // ── cascade computation ────────────────────────────────────
    /**
     * Compute light-space matrices for a directional light.
     * @param cameraViewProj  Combined camera view-projection (for frustum corners)
     * @param lightDir        Normalised world-space light direction (points toward light)
     * @param cameraNear      Camera near plane
     * @param cameraFar       Camera far plane (maximum shadow distance)
     * @returns Per-cascade LightSpaceMatrix; length == CascadeCount().
     */
    std::vector<LightSpaceMatrix> ComputeCascades(
        const Math::Mat4& cameraViewProj,
        const Math::Vec3& lightDir,
        float cameraNear,
        float cameraFar) const;

    /**
     * Compute split distances only (useful for shader upload).
     * Returns numCascades+1 values: [near, split1, split2, ..., far]
     */
    std::vector<float> ComputeSplitDistances(float near, float far) const;

    // ── shadow map registry ────────────────────────────────────
    uint32_t RegisterLight(uint32_t lightId);
    void     UnregisterLight(uint32_t lightId);
    const ShadowMap* GetShadowMap(uint32_t lightId) const;
    ShadowMap*       GetShadowMap(uint32_t lightId);
    std::vector<uint32_t> RegisteredLights() const;

    // ── utility ───────────────────────────────────────────────
    /// Compute a tight orthographic projection enclosing frustum corners.
    static LightSpaceMatrix FrustumToLightSpace(
        const std::array<Math::Vec3, 8>& frustumCorners,
        const Math::Vec3& lightDir,
        float depthBias,
        float nearOffset = 50.0f);

    /// Extract world-space frustum corners from a view-projection matrix.
    static std::array<Math::Vec3, 8> ExtractFrustumCorners(
        const Math::Mat4& invViewProj);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
