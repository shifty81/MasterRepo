#pragma once
/**
 * @file ChunkSystem.h
 * @brief World chunk manager: load/unload, LOD tiers, priority streaming, neighbour queries.
 *
 * Features:
 *   - 3D grid of fixed-size chunks (configurable chunk world size)
 *   - Focus point: set observer position to compute visible/load/unload sets
 *   - Load radius, unload radius (hysteresis), LOD tier distances
 *   - Priority queue: closer chunks load first; background thread-friendly
 *   - Chunk states: Unloaded → Loading → Loaded → Unloading
 *   - On-load / on-unload callbacks per chunk
 *   - Data payload: arbitrary per-chunk user data pointer
 *   - Neighbour query: GetNeighbour(cx,cy,cz, direction)
 *   - World-to-chunk and chunk-to-world coordinate conversion
 *   - Chunk dirty flag for runtime modification
 *   - Streaming budget: max chunks loaded simultaneously
 *
 * Typical usage:
 * @code
 *   ChunkSystem cs;
 *   cs.Init({64.f, 64.f, 64.f}, 128);  // chunk size, budget
 *   cs.SetOnLoad  ([](ChunkCoord c){ LoadChunkData(c); });
 *   cs.SetOnUnload([](ChunkCoord c){ FreeChunkData(c); });
 *   cs.SetFocusPoint({playerX, playerY, playerZ});
 *   cs.SetLoadRadius(3);
 *   cs.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct ChunkCoord {
    int32_t x{0}, y{0}, z{0};
    bool operator==(const ChunkCoord& o) const { return x==o.x&&y==o.y&&z==o.z; }
};

enum class ChunkState : uint8_t { Unloaded=0, Queued, Loading, Loaded, Unloading };

enum class ChunkLOD : uint8_t { High=0, Medium, Low, VeryLow };

struct ChunkInfo {
    ChunkCoord  coord;
    ChunkState  state   {ChunkState::Unloaded};
    ChunkLOD    lod     {ChunkLOD::High};
    bool        dirty   {false};
    void*       userData{nullptr};
    float       distFromFocus{0.f};
};

struct ChunkSystemConfig {
    float    chunkSizeX{64.f};
    float    chunkSizeY{64.f};
    float    chunkSizeZ{64.f};
    uint32_t budget     {128};   ///< max loaded chunks
    uint32_t loadRadius {3};
    uint32_t unloadRadius{4};
    float    lodDistances[4]{32.f,64.f,128.f,256.f};
};

class ChunkSystem {
public:
    ChunkSystem();
    ~ChunkSystem();

    void Init    (const ChunkSystemConfig& cfg);
    void Shutdown();
    void Tick    (float dt);

    // Observer
    void SetFocusPoint(const float pos[3]);
    void SetLoadRadius  (uint32_t r);
    void SetUnloadRadius(uint32_t r);

    // Coordinate conversion
    ChunkCoord WorldToChunk (const float worldPos[3]) const;
    void       ChunkToWorld (const ChunkCoord& c, float out[3]) const;

    // Chunk queries
    const ChunkInfo* GetChunk    (const ChunkCoord& c) const;
    ChunkInfo*       GetChunkMut (const ChunkCoord& c);
    bool             IsLoaded    (const ChunkCoord& c) const;
    ChunkState       GetState    (const ChunkCoord& c) const;

    std::vector<ChunkCoord> GetLoadedChunks()                    const;
    std::vector<ChunkCoord> GetChunksInRadius(const float pos[3], uint32_t radius) const;
    ChunkCoord              GetNeighbour(const ChunkCoord& c, int32_t dx, int32_t dy, int32_t dz) const;

    // Manual load/unload
    void RequestLoad  (const ChunkCoord& c);
    void RequestUnload(const ChunkCoord& c);

    // Chunk data
    void  SetUserData(const ChunkCoord& c, void* data);
    void* GetUserData(const ChunkCoord& c) const;
    void  MarkDirty  (const ChunkCoord& c);

    // Stats
    uint32_t LoadedCount()  const;
    uint32_t QueuedCount()  const;
    uint32_t BudgetUsed()   const;

    // Callbacks
    using ChunkCb = std::function<void(const ChunkCoord&)>;
    void SetOnLoad      (ChunkCb cb);
    void SetOnUnload    (ChunkCb cb);
    void SetOnLODChange (std::function<void(const ChunkCoord&, ChunkLOD)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
