#pragma once
/**
 * @file VoxelWorld.h
 * @brief Chunk-based voxel grid with greedy mesh generation and streaming.
 *
 * VoxelWorld manages a 3D array of voxels divided into fixed-size chunks:
 *   - VoxelType: 8-bit material ID (0 = air)
 *   - VoxelChunk: NxNxN block of voxels with a dirty flag
 *   - VoxelMesh: greedy-merged quad list + vertex/index output for rendering
 *   - VoxelWorld: set/get voxel, chunk streaming around a focus point,
 *     RayCast for pick/place, flood-fill for cavity detection
 */

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include "Engine/Math/Math.h"

namespace Engine {

using VoxelType = uint8_t;
constexpr VoxelType kAir = 0;

// ── Chunk coordinates ─────────────────────────────────────────────────────
struct VoxelChunkCoord {
    int32_t cx{0}, cy{0}, cz{0};
    bool operator==(const VoxelChunkCoord& o) const {
        return cx==o.cx && cy==o.cy && cz==o.cz;
    }
};
struct VoxelChunkCoordHash {
    size_t operator()(const VoxelChunkCoord& c) const {
        size_t h = std::hash<int32_t>()(c.cx);
        h ^= std::hash<int32_t>()(c.cy) + 0x9e3779b9 + (h<<6) + (h>>2);
        h ^= std::hash<int32_t>()(c.cz) + 0x9e3779b9 + (h<<6) + (h>>2);
        return h;
    }
};

// ── Voxel chunk ───────────────────────────────────────────────────────────
constexpr uint32_t kVoxelChunkSize = 16;

struct VoxelChunk {
    VoxelChunkCoord coord;
    VoxelType voxels[kVoxelChunkSize][kVoxelChunkSize][kVoxelChunkSize]{};
    bool dirty{true};

    VoxelType Get(uint32_t x, uint32_t y, uint32_t z) const;
    void      Set(uint32_t x, uint32_t y, uint32_t z, VoxelType t);
    bool      IsAir(uint32_t x, uint32_t y, uint32_t z) const;
};

// ── Mesh output ───────────────────────────────────────────────────────────
struct VoxelVertex {
    Math::Vec3 pos;
    Math::Vec3 normal;
    float      u{0.0f}, v{0.0f};
    VoxelType  material{0};
};

struct VoxelMesh {
    std::vector<VoxelVertex>  vertices;
    std::vector<uint32_t>     indices;
    VoxelChunkCoord           chunkCoord;
    bool                      upToDate{false};
};

// ── Ray cast ─────────────────────────────────────────────────────────────
struct VoxelHit {
    bool         hit{false};
    Math::Vec3   point;
    Math::Vec3   normal;      ///< Face normal of hit voxel
    int32_t      vx{0},vy{0},vz{0}; ///< World voxel coordinates
    VoxelType    material{0};
    float        distance{0.0f};
};

// ── Mesh generation callback ───────────────────────────────────────────────
using MeshReadyCb = std::function<void(const VoxelMesh&)>;

// ── World ─────────────────────────────────────────────────────────────────
class VoxelWorld {
public:
    VoxelWorld();
    ~VoxelWorld();

    // ── voxel access ──────────────────────────────────────────
    VoxelType GetVoxel(int32_t wx, int32_t wy, int32_t wz) const;
    void      SetVoxel(int32_t wx, int32_t wy, int32_t wz, VoxelType t);
    bool      IsAir(int32_t wx, int32_t wy, int32_t wz) const;

    // ── chunk streaming ───────────────────────────────────────
    void SetStreamRadius(int32_t radius);
    void UpdateStreaming(float viewX, float viewY, float viewZ);
    size_t LoadedChunkCount() const;

    const VoxelChunk* GetChunk(VoxelChunkCoord coord) const;
    VoxelChunk*       GetChunk(VoxelChunkCoord coord);

    VoxelChunkCoord WorldToChunk(int32_t wx, int32_t wy, int32_t wz) const;

    // ── mesh building ─────────────────────────────────────────
    /// Rebuild greedy mesh for all dirty chunks.
    void RebuildDirtyMeshes();
    /// Get (or build) mesh for a specific chunk.
    const VoxelMesh* GetMesh(VoxelChunkCoord coord) const;
    /// Register callback fired when a mesh becomes ready.
    void OnMeshReady(MeshReadyCb cb);

    // ── ray cast ──────────────────────────────────────────────
    VoxelHit RayCast(const Math::Vec3& origin,
                     const Math::Vec3& direction,
                     float maxDistance = 64.0f) const;

    // ── flood fill ────────────────────────────────────────────
    /// Returns all air voxel positions reachable from (wx,wy,wz) up to maxVoxels.
    std::vector<std::tuple<int32_t,int32_t,int32_t>>
        FloodFill(int32_t wx, int32_t wy, int32_t wz,
                  uint32_t maxVoxels = 4096) const;

    // ── fill sphere ───────────────────────────────────────────
    void FillSphere(int32_t cx, int32_t cy, int32_t cz, float radius, VoxelType t);

private:
    struct Impl;
    Impl* m_impl{nullptr};

    VoxelChunk* loadChunk(VoxelChunkCoord coord);
    void        unloadChunk(VoxelChunkCoord coord);
    void        buildMesh(VoxelChunk& chunk);
};

} // namespace Engine
