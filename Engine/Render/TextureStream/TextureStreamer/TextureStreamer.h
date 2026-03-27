#pragma once
/**
 * @file TextureStreamer.h
 * @brief Mip-level texture streaming with memory budget, priority queue, and LRU eviction.
 *
 * Features:
 *   - RegisterTexture(id, fullMips, bytePerMip[]): register a texture and its mip sizes
 *   - UnregisterTexture(id)
 *   - SetResidency(id, mipLevel): request a specific highest-loaded mip (0 = full res)
 *   - Update(cameraPos, frustumPlanes[24]): prioritise textures in view, trigger async loads
 *   - Tick(dt): advance pending requests; calls OnMipLoaded callback when mip is ready
 *   - GetResidentMip(id) → uint32_t: currently loaded highest-resolution mip
 *   - SetBudgetBytes(bytes): GPU memory budget; triggers LRU eviction when exceeded
 *   - GetUsedBytes() → uint64_t
 *   - SetOnMipLoaded(cb): callback when a mip becomes available
 *   - SetOnMipEvicted(cb): callback when a mip is evicted
 *   - SetUrgencyBias(id, bias): increase load priority for important textures
 *   - ForceResident(id, mipLevel): immediately mark mip as resident (no actual GPU upload)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct TSVec3 { float x, y, z; };

class TextureStreamer {
public:
    TextureStreamer();
    ~TextureStreamer();

    void Init    (uint64_t budgetBytes=256*1024*1024ULL);
    void Shutdown();
    void Reset   ();

    // Texture registry
    void RegisterTexture  (uint32_t id, uint32_t mipCount,
                           const uint64_t* bytesPerMip);   ///< bytesPerMip[0]=full res
    void UnregisterTexture(uint32_t id);

    // Residency control
    void SetResidency (uint32_t id, uint32_t mipLevel);
    void ForceResident(uint32_t id, uint32_t mipLevel);

    // Per-frame update
    void Update(TSVec3 cameraPos, const float* frustumPlanes); ///< planes[24]
    void Tick  (float dt);

    // Query
    uint32_t GetResidentMip  (uint32_t id) const;
    uint64_t GetUsedBytes    ()            const;
    uint64_t GetBudgetBytes  ()            const;
    uint32_t GetPendingCount ()            const;

    // Config
    void SetBudgetBytes (uint64_t bytes);
    void SetUrgencyBias (uint32_t id, float bias);  ///< >1 = higher priority

    // Callbacks
    void SetOnMipLoaded (std::function<void(uint32_t id, uint32_t mip)> cb);
    void SetOnMipEvicted(std::function<void(uint32_t id, uint32_t mip)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
