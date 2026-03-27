#pragma once
/**
 * @file TextureAtlas.h
 * @brief Pack sub-textures into a single atlas; store/lookup UV rects.
 *
 * Features:
 *   - Create(atlasId, width, height) → bool
 *   - Destroy(atlasId)
 *   - Pack(atlasId, spriteId, spriteW, spriteH) → bool: shelf-packing
 *   - Remove(atlasId, spriteId)
 *   - GetUVRect(atlasId, spriteId, outU0, outV0, outU1, outV1) → bool
 *   - GetPixelRect(atlasId, spriteId, outX, outY, outW, outH) → bool
 *   - GetAtlasSize(atlasId, outW, outH)
 *   - GetUsedArea(atlasId) → float: fraction [0,1] of total atlas used
 *   - GetSpriteCount(atlasId) → uint32_t
 *   - PackBatch(atlasId, ids[], ws[], hs[], n) → uint32_t packed
 *   - GetAllSprites(atlasId, out[]) → uint32_t
 *   - SetOnFull(atlasId, cb): callback when a pack fails due to no space
 *   - Reset(atlasId): clear all sprites but keep atlas
 *   - Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

class TextureAtlas {
public:
    TextureAtlas();
    ~TextureAtlas();

    void Init    ();
    void Shutdown();

    // Atlas management
    bool Create (uint32_t atlasId, uint32_t width, uint32_t height);
    void Destroy(uint32_t atlasId);
    void Reset  (uint32_t atlasId); // clear sprites, keep dimensions

    // Packing
    bool     Pack     (uint32_t atlasId, uint32_t spriteId,
                       uint32_t w, uint32_t h);
    uint32_t PackBatch(uint32_t atlasId,
                       const std::vector<uint32_t>& ids,
                       const std::vector<uint32_t>& ws,
                       const std::vector<uint32_t>& hs);
    void     Remove   (uint32_t atlasId, uint32_t spriteId);

    // Query — UV (normalized [0,1])
    bool GetUVRect   (uint32_t atlasId, uint32_t spriteId,
                      float& u0, float& v0, float& u1, float& v1) const;
    // Query — pixel
    bool GetPixelRect(uint32_t atlasId, uint32_t spriteId,
                      uint32_t& x, uint32_t& y,
                      uint32_t& w, uint32_t& h) const;

    // Atlas info
    void     GetAtlasSize  (uint32_t atlasId, uint32_t& w, uint32_t& h) const;
    float    GetUsedArea   (uint32_t atlasId) const;
    uint32_t GetSpriteCount(uint32_t atlasId) const;
    uint32_t GetAllSprites (uint32_t atlasId, std::vector<uint32_t>& out) const;

    // Callback
    void SetOnFull(uint32_t atlasId, std::function<void()> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
