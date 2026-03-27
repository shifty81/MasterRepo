#pragma once
/**
 * @file ScreenSpaceAO.h
 * @brief Screen-space ambient occlusion: hemisphere sampling, blur, compositing.
 *
 * Features:
 *   - SetResolution(w, h): AO buffer resolution (may differ from screen)
 *   - SetRadius(r): world-space sample hemisphere radius
 *   - SetBias(b): depth bias to avoid self-occlusion
 *   - SetIntensity(i): occlusion strength multiplier
 *   - SetSampleCount(n): hemisphere sample count (4–64)
 *   - SetBlurRadius(px): bilateral blur kernel size
 *   - ComputeAO(depthBuffer[], normalBuffer[], viewMat, projMat,
 *               outAO[]) → bool: CPU reference implementation
 *   - Blur(inAO[], outAO[], depthBuffer[]): bilateral/box blur pass
 *   - Composite(inColor[], inAO[], outColor[]): multiply AO into colour
 *   - SetNoiseTexture(data, w, h): jitter hemisphere rotations
 *   - SetEnabled(on) / IsEnabled() → bool
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <vector>

namespace Engine {

struct SSAOMat4 { float m[16]; };

class ScreenSpaceAO {
public:
    ScreenSpaceAO();
    ~ScreenSpaceAO();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Config
    void SetResolution (uint32_t w, uint32_t h);
    void SetRadius     (float r);
    void SetBias       (float b);
    void SetIntensity  (float i);
    void SetSampleCount(uint32_t n);
    void SetBlurRadius (uint32_t px);
    void SetEnabled    (bool on);
    bool IsEnabled     () const;

    // Noise jitter
    void SetNoiseTexture(const float* data, uint32_t w, uint32_t h);

    // Compute passes
    bool ComputeAO (const float* depthBuf, const float* normalBuf,
                    const SSAOMat4& viewMat, const SSAOMat4& projMat,
                    std::vector<float>& outAO) const;
    void Blur      (const std::vector<float>& inAO,
                    std::vector<float>& outAO,
                    const float* depthBuf) const;
    void Composite (const std::vector<float>& inColor,
                    const std::vector<float>& inAO,
                    std::vector<float>& outColor) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
