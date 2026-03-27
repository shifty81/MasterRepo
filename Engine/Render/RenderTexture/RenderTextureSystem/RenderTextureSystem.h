#pragma once
/**
 * @file RenderTextureSystem.h
 * @brief Off-screen render texture management: create, bind, blit, mip, format.
 *
 * Features:
 *   - PixelFormat: RGBA8, RGBA16F, RGBA32F, Depth24, Depth32F
 *   - CreateTexture(id, width, height, format, mips) → bool
 *   - DestroyTexture(id)
 *   - Resize(id, width, height)
 *   - Clear(id, r, g, b, a)
 *   - Blit(srcId, dstId): full-texture copy
 *   - BlitRegion(srcId, dstId, srcX, srcY, srcW, srcH, dstX, dstY, dstW, dstH)
 *   - GenerateMips(id)
 *   - ReadPixels(id, x, y, w, h, outData[]) → bool
 *   - WritePixels(id, x, y, w, h, inData[]) → bool
 *   - GetWidth(id) / GetHeight(id) → uint32_t
 *   - GetFormat(id) → PixelFormat
 *   - GetMipCount(id) → uint32_t
 *   - SetFilterMode(id, linear): true=bilinear, false=nearest
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <vector>

namespace Engine {

enum class RTPixelFormat : uint8_t { RGBA8, RGBA16F, RGBA32F, Depth24, Depth32F };

class RenderTextureSystem {
public:
    RenderTextureSystem();
    ~RenderTextureSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Texture management
    bool CreateTexture (uint32_t id, uint32_t width, uint32_t height,
                        RTPixelFormat format = RTPixelFormat::RGBA8,
                        uint32_t mips = 1);
    void DestroyTexture(uint32_t id);
    void Resize        (uint32_t id, uint32_t width, uint32_t height);

    // Operations
    void Clear          (uint32_t id, float r, float g, float b, float a = 1.f);
    void Blit           (uint32_t srcId, uint32_t dstId);
    void BlitRegion     (uint32_t srcId, uint32_t dstId,
                         uint32_t srcX, uint32_t srcY, uint32_t srcW, uint32_t srcH,
                         uint32_t dstX, uint32_t dstY, uint32_t dstW, uint32_t dstH);
    void GenerateMips   (uint32_t id);

    // Pixel I/O
    bool ReadPixels (uint32_t id, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                     std::vector<float>& outData) const;
    bool WritePixels(uint32_t id, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                     const std::vector<float>& inData);

    // Queries
    uint32_t     GetWidth   (uint32_t id) const;
    uint32_t     GetHeight  (uint32_t id) const;
    RTPixelFormat GetFormat  (uint32_t id) const;
    uint32_t     GetMipCount(uint32_t id) const;

    // Filter
    void SetFilterMode(uint32_t id, bool linear);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
