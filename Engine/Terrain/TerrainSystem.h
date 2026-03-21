#pragma once
/**
 * @file TerrainSystem.h
 * @brief Heightmap-based terrain: chunk streaming, LOD, normals, ray-vs-terrain.
 *
 * TerrainSystem manages a tiled heightmap world:
 *   - HeightmapChunk: NxN cell tile with heights, normals, LOD level
 *   - ChunkCache: load/unload chunks around a focal point (streaming)
 *   - HeightQuery: sample height + normal at arbitrary world XZ
 *   - RayCast: find world XYZ intersection of a ray with the terrain surface
 *   - LODPolicy: select detail level based on camera distance
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include "Engine/Math/Math.h"

namespace Engine {

// ── Chunk ─────────────────────────────────────────────────────────────────
struct ChunkCoord {
    int32_t cx{0}, cz{0};
    bool operator==(const ChunkCoord& o) const { return cx == o.cx && cz == o.cz; }
};

struct ChunkCoordHash {
    size_t operator()(const ChunkCoord& c) const {
        return std::hash<int64_t>()(static_cast<int64_t>(c.cx) << 32 | static_cast<uint32_t>(c.cz));
    }
};

struct TerrainChunk {
    ChunkCoord  coord;
    uint32_t    resolution{65};   ///< Vertices per side (must be 2^n+1)
    float       cellSize{1.0f};   ///< World units per cell
    float       heightScale{64.0f};
    int32_t     lodLevel{0};      ///< 0 = highest detail
    bool        loaded{false};

    std::vector<float>             heights;   ///< resolution*resolution
    std::vector<Engine::Math::Vec3> normals;  ///< per-vertex normals

    float WorldX() const { return static_cast<float>(coord.cx) * (resolution - 1) * cellSize; }
    float WorldZ() const { return static_cast<float>(coord.cz) * (resolution - 1) * cellSize; }
    float ChunkWorldSize() const { return (resolution - 1) * cellSize; }

    float SampleHeight(float localX, float localZ) const;
    Engine::Math::Vec3 SampleNormal(float localX, float localZ) const;
    void  ComputeNormals();
};

// ── LOD policy ────────────────────────────────────────────────────────────
struct LODPolicy {
    std::vector<float> distanceThresholds; ///< [lod0_maxDist, lod1_maxDist, ...]
    uint32_t           maxLOD{4};

    int32_t SelectLOD(float distance) const;
};

// ── Ray-terrain hit ───────────────────────────────────────────────────────
struct TerrainHit {
    bool               hit{false};
    Engine::Math::Vec3 point;
    Engine::Math::Vec3 normal;
    float              distance{0.0f};
    ChunkCoord         chunk;
};

// ── Heightmap source callback ─────────────────────────────────────────────
using HeightmapSourceCb = std::function<
    void(TerrainChunk& chunk)>;  ///< Fill chunk.heights from e.g. noise/file

// ── System ───────────────────────────────────────────────────────────────
class TerrainSystem {
public:
    TerrainSystem();
    ~TerrainSystem();

    // ── config ────────────────────────────────────────────────
    void SetChunkResolution(uint32_t res);
    void SetCellSize(float size);
    void SetHeightScale(float scale);
    void SetStreamRadius(int32_t chunkRadius);   ///< Load chunks within radius
    void SetHeightSource(HeightmapSourceCb cb);
    void SetLODPolicy(const LODPolicy& policy);

    // ── streaming ─────────────────────────────────────────────
    /// Call each frame with viewer world position to stream chunks in/out.
    void UpdateStreaming(float viewX, float viewZ);
    size_t LoadedChunkCount() const;

    // ── chunk access ──────────────────────────────────────────
    const TerrainChunk* GetChunk(ChunkCoord coord) const;
    TerrainChunk*       GetChunk(ChunkCoord coord);
    std::vector<ChunkCoord> LoadedChunks() const;

    // ── coordinate conversion ─────────────────────────────────
    ChunkCoord WorldToChunk(float wx, float wz) const;

    // ── height query ──────────────────────────────────────────
    /// Sample terrain height at arbitrary world (wx, wz). Returns 0 if not loaded.
    float SampleHeight(float wx, float wz) const;
    /// Sample normal at world position.
    Engine::Math::Vec3 SampleNormal(float wx, float wz) const;

    // ── ray cast ──────────────────────────────────────────────
    TerrainHit RayCast(const Engine::Math::Vec3& origin,
                       const Engine::Math::Vec3& direction,
                       float maxDistance = 1000.0f,
                       float stepSize    = 0.5f) const;

    // ── modification ──────────────────────────────────────────
    /// Sculpt: add @p delta to heights within @p radius of world position.
    void Sculpt(float wx, float wz, float radius, float delta);

private:
    struct Impl;
    Impl* m_impl{nullptr};

    TerrainChunk* loadChunk(ChunkCoord coord);
    void          unloadChunk(ChunkCoord coord);
};

} // namespace Engine
