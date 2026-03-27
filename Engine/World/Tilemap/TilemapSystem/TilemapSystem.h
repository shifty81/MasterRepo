#pragma once
/**
 * @file TilemapSystem.h
 * @brief 2D tilemap: layers, tile-id grid, tileset atlas UVs, collision mask, queries.
 *
 * Features:
 *   - TilemapDesc: width, height (tiles), tileWidth/Height (px), layerCount
 *   - Layers: ordered draw stack; each layer has a uint16_t tile-id grid
 *   - Tile id 0 = empty (transparent)
 *   - TilesetAtlas: texture dimensions, tile count, computes UV rect per tile id
 *   - Collision mask: per-tile-id bitmask (up/down/left/right/diagonal passable)
 *   - SetTile(layerIdx, x, y, tileId)
 *   - GetTile(layerIdx, x, y) → tileId
 *   - GetUVRect(tileId) → {u0,v0,u1,v1}
 *   - IsPassable(x, y, direction) → bool
 *   - GetTileAtWorld(worldX, worldY) → tile grid coords
 *   - Multiple tilemaps
 *   - Layer visibility toggle
 *   - Dirty rect tracking for incremental re-upload
 *
 * Typical usage:
 * @code
 *   TilemapSystem ts;
 *   ts.Init();
 *   TilemapDesc d; d.width=32; d.height=24; d.tileWidth=16; d.tileHeight=16; d.layerCount=3;
 *   uint32_t id = ts.Create(d);
 *   ts.SetAtlas(id, 512, 512, 16, 16);
 *   ts.SetTile(id, 0, 5, 3, 42);
 *   auto uv = ts.GetUVRect(id, 42);
 *   bool ok = ts.IsPassable(id, 5, 3, 0);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct TilemapDesc {
    uint32_t width     {32};
    uint32_t height    {24};
    uint32_t tileWidth {16};
    uint32_t tileHeight{16};
    uint32_t layerCount{1};
};

struct TileUVRect { float u0,v0,u1,v1; };

class TilemapSystem {
public:
    TilemapSystem();
    ~TilemapSystem();

    void Init    ();
    void Shutdown();

    // Tilemap lifecycle
    uint32_t Create (const TilemapDesc& desc);
    void     Destroy(uint32_t id);
    bool     Has    (uint32_t id) const;

    // Atlas
    void SetAtlas(uint32_t id, uint32_t texW, uint32_t texH,
                   uint32_t tilePxW, uint32_t tilePxH);

    // Tile access
    void     SetTile(uint32_t id, uint32_t layer, int32_t x, int32_t y, uint16_t tileId);
    uint16_t GetTile(uint32_t id, uint32_t layer, int32_t x, int32_t y) const;
    void     FillRect(uint32_t id, uint32_t layer,
                       int32_t x, int32_t y, int32_t w, int32_t h, uint16_t tileId);

    // UV
    TileUVRect GetUVRect(uint32_t id, uint16_t tileId) const;

    // Collision
    void SetCollisionMask(uint32_t id, uint16_t tileId, uint8_t mask);
    uint8_t GetCollisionMask(uint32_t id, uint16_t tileId) const;
    bool IsPassable(uint32_t id, int32_t x, int32_t y, uint8_t direction=0) const;

    // World queries
    void GetTileAtWorld(uint32_t id, float worldX, float worldY,
                         int32_t& outX, int32_t& outY) const;
    void GetWorldPos   (uint32_t id, int32_t tx, int32_t ty,
                         float& worldX, float& worldY) const;

    // Layers
    void SetLayerVisible(uint32_t id, uint32_t layer, bool visible);
    bool IsLayerVisible (uint32_t id, uint32_t layer) const;

    // Dirty
    bool IsDirty(uint32_t id, uint32_t layer) const;
    void ClearDirty(uint32_t id, uint32_t layer);

    // Callback
    void SetOnTileChanged(std::function<void(uint32_t tilemapId, uint32_t layer,
                                              int32_t x, int32_t y)> cb);

    std::vector<uint32_t> GetAll() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
