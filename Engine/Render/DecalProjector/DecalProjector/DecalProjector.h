#pragma once
/**
 * @file DecalProjector.h
 * @brief Projects texture decals onto world geometry: placement, fade, atlas, priority.
 *
 * Features:
 *   - SpawnDecal(id, x, y, z, nx, ny, nz, atlasId, w, h, life) → bool
 *   - RemoveDecal(id) / RemoveAll()
 *   - SetAtlas(atlasId, cols, rows): UV sub-rect lookup by atlasId+tileIndex
 *   - GetDecalCount() → uint32_t
 *   - Tick(dt): age decals, fade out, expire
 *   - SetFadeDuration(secs): fade-out time before expiry
 *   - SetMaxDecals(n): oldest removed on overflow
 *   - SetOnExpire(cb): callback(id) when decal lifetime ends
 *   - GetDecalPosition(id, outX, outY, outZ)
 *   - GetDecalNormal(id, outNX, outNY, outNZ)
 *   - GetDecalAlpha(id) → float: 0=faded, 1=full
 *   - SetDecalTile(id, tileIndex): change atlas tile
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>

namespace Engine {

class DecalProjector {
public:
    DecalProjector();
    ~DecalProjector();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Atlas setup
    void SetAtlas(uint32_t atlasId, uint32_t cols, uint32_t rows);

    // Decal lifecycle
    bool SpawnDecal(uint32_t id, float x, float y, float z,
                    float nx, float ny, float nz,
                    uint32_t atlasId, float w, float h, float life);
    void RemoveDecal(uint32_t id);
    void RemoveAll  ();

    // Per-frame
    void Tick(float dt);

    // Config
    void SetFadeDuration(float secs);
    void SetMaxDecals   (uint32_t n);

    // Query
    uint32_t GetDecalCount() const;
    void     GetDecalPosition(uint32_t id, float& x, float& y, float& z) const;
    void     GetDecalNormal  (uint32_t id, float& nx, float& ny, float& nz) const;
    float    GetDecalAlpha   (uint32_t id) const;
    void     SetDecalTile    (uint32_t id, uint32_t tileIndex);

    // Callback
    void SetOnExpire(std::function<void(uint32_t id)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
