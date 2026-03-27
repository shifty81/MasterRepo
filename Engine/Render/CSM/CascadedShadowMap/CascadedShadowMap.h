#pragma once
/**
 * @file CascadedShadowMap.h
 * @brief Cascaded shadow map generation: frustum split, per-cascade view/proj, PCF sampling.
 *
 * Features:
 *   - SetCascadeCount(n): number of shadow cascades (1–4)
 *   - SetLightDir(dir): normalised directional light direction
 *   - SetCameraFrustum(fovY, aspect, zNear, zFar, camPos, camFwd, camUp): camera params
 *   - ComputeSplits(): compute per-cascade frustum split depths (PSSM or uniform)
 *   - ComputeLightMatrices(): build per-cascade view+ortho matrices
 *   - GetLightViewMatrix(cascade) → Mat4
 *   - GetLightProjMatrix(cascade) → Mat4
 *   - GetSplitDepth(cascade) → float: view-space far distance for cascade
 *   - SetShadowMapSize(width, height): resolution per cascade
 *   - SetLambda(l): blend factor [0=uniform, 1=logarithmic] for split distribution
 *   - SetDepthBias(cascade, bias): per-cascade depth bias to reduce self-shadow
 *   - GetPCFKernelSize() → uint32_t / SetPCFKernelSize(n)
 *   - SetBlendBand(width): cascade blend overlap width in world space
 *   - SampleShadow(worldPos, cascade) → float [0,1]: PCF shadow sample (CPU reference)
 *   - Reset() / Shutdown()
 */

#include <array>
#include <cstdint>

namespace Engine {

struct CSMVec3  { float x,y,z; };
struct CSMMat4  { float m[16]; };

class CascadedShadowMap {
public:
    static constexpr uint32_t kMaxCascades = 4;

    CascadedShadowMap();
    ~CascadedShadowMap();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Setup
    void SetCascadeCount(uint32_t n);
    void SetLightDir    (CSMVec3 dir);
    void SetCameraFrustum(float fovY, float aspect, float zNear, float zFar,
                          CSMVec3 camPos, CSMVec3 camFwd, CSMVec3 camUp);
    void SetShadowMapSize(uint32_t w, uint32_t h);
    void SetLambda       (float l);
    void SetDepthBias    (uint32_t cascade, float bias);
    void SetPCFKernelSize(uint32_t n);
    void SetBlendBand    (float width);

    // Computation
    void ComputeSplits        ();
    void ComputeLightMatrices ();

    // Output
    CSMMat4  GetLightViewMatrix(uint32_t cascade) const;
    CSMMat4  GetLightProjMatrix(uint32_t cascade) const;
    float    GetSplitDepth    (uint32_t cascade) const;
    uint32_t GetPCFKernelSize () const;
    uint32_t GetCascadeCount  () const;

    // CPU reference shadow sample
    float SampleShadow(CSMVec3 worldPos, uint32_t cascade) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
