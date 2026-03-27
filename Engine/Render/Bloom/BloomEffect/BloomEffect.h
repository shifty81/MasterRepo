#pragma once
/**
 * @file BloomEffect.h
 * @brief HDR bloom post-process: threshold extraction, Gaussian blur pyramid, upsample blend.
 *
 * Features:
 *   - Process(srcPixels, width, height) → output float-RGBA buffer (in-place or separate)
 *   - SetThreshold(t): luminance threshold for bright-pass extraction
 *   - SetIntensity(k): final blend weight of bloom on source image
 *   - SetBlurRadius(r): half-kernel size for Gaussian passes
 *   - SetMipLevels(n): number of downsample / upsample pyramid levels
 *   - SetDirtMask(maskPixels, w, h): optional lens-dirt texture multiplied into bloom
 *   - SetDirtIntensity(k)
 *   - EnableKawaseMode(on): use Kawase dual-filter instead of separable Gaussian
 *   - GetLastBrightPassPixels() → float*: inspect bright-pass result
 *   - GetOutputPixels() → float*: access final blended output
 *   - GetWidth() / GetHeight()
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <vector>

namespace Engine {

class BloomEffect {
public:
    BloomEffect();
    ~BloomEffect();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Main API
    void Process(const float* srcRGBA, uint32_t width, uint32_t height);

    // Configuration
    void SetThreshold     (float t);
    void SetIntensity     (float k);
    void SetBlurRadius    (uint32_t r);
    void SetMipLevels     (uint32_t n);
    void SetDirtMask      (const float* maskRGBA, uint32_t w, uint32_t h);
    void SetDirtIntensity (float k);
    void EnableKawaseMode (bool on);

    // Output access
    const float* GetLastBrightPassPixels() const;
    const float* GetOutputPixels        () const;
    uint32_t     GetWidth               () const;
    uint32_t     GetHeight              () const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
