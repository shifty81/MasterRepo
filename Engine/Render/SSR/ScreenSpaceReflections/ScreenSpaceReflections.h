#pragma once
/**
 * @file ScreenSpaceReflections.h
 * @brief CPU-side screen-space reflection ray-marcher with roughness fade and cubemap fallback.
 *
 * Features:
 *   - RayMarch(origin, dir, depthBuf, normalBuf, width, height) → hit UV or invalid
 *   - Roughness fade: reflections attenuate as surface roughness increases
 *   - Max ray steps, step size, stride jitter (blue-noise offset) configurable
 *   - Edge fade: reflections fade near screen borders to avoid hard clipping
 *   - CubemapFallback: when ray misses, blend to a supplied environment colour
 *   - HitConfidence(): returned per-hit quality weight for TAA blend
 *   - SetProjection(fov, aspect, near, far) for view-space reconstruction
 *   - Blit(srcColour, reflectBuf, roughnessBuf, output, w, h): composite pass
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>

namespace Engine {

struct Vec3  { float x,y,z; };
struct Vec2  { float x,y; };
struct Mat4  { float m[16]; };

struct SSRHit {
    Vec2  uv;           ///< screen-space UV of hit point
    float confidence;   ///< [0,1] quality weight
    bool  valid;        ///< true if ray found a hit
};

class ScreenSpaceReflections {
public:
    ScreenSpaceReflections();
    ~ScreenSpaceReflections();

    void Init    (uint32_t maxSteps=64, float stepSize=0.02f);
    void Shutdown();
    void Reset   ();

    // Projection setup
    void SetProjection(float fovY, float aspect, float nearZ, float farZ);
    void SetViewMatrix(const Mat4& view);

    // Ray march a single reflection ray
    // depthBuf and normalBuf are linearised float arrays [w*h]
    SSRHit RayMarch(Vec3 originVS, Vec3 dirVS,
                    const float* depthBuf, const float* normalBuf,
                    uint32_t w, uint32_t h) const;

    // Batch composite: for each pixel sample reflection colour from colourBuf,
    // blend using roughnessBuf and write to outputBuf (all float RGBA, row-major)
    void Blit(const float* colourBuf, const float* reflectBuf,
              const float* roughnessBuf, float* outputBuf,
              uint32_t w, uint32_t h) const;

    // Config
    void SetMaxSteps   (uint32_t steps);
    void SetStepSize   (float size);
    void SetMaxDistance(float dist);
    void SetEdgeFade   (float margin); ///< UV fraction to fade at borders
    void SetRoughnessCutoff(float r);  ///< roughness above which SSR weight → 0
    void SetJitter     (bool enable);

    // Fallback colour for misses (constant env colour)
    void SetFallbackColour(float r, float g, float b);

    uint32_t MaxSteps()  const;
    float    StepSize()  const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
