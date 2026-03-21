#pragma once
/**
 * @file WorldChunkGen.h
 * @brief Chunk-based procedural world generator with biome-aware heightmaps.
 *
 * WorldChunkGen divides the world into uniform square chunks and generates
 * each chunk's terrain on demand:
 *
 *   - ChunkCoord: integer (cx, cz) chunk address; y is the vertical dimension.
 *   - HeightSample: height value + biome id + surface material tag.
 *   - WorldChunk: width×depth grid of HeightSamples + decoration list.
 *   - Decoration: a placed object (tree, rock, structure) at a local offset.
 *   - Generator config: chunk size, octave noise params, biome seed, sea level.
 *   - GenerateChunk(cx, cz): produces one WorldChunk deterministically.
 *   - LoadRadius(cx, cz, r): generate all chunks within Manhattan radius r.
 *   - Cache: keeps the most-recently-used N chunks in memory; evicts oldest.
 *   - OnChunkReady: callback fired after each chunk is generated.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace PCG {

// ── Chunk coordinates ─────────────────────────────────────────────────────
struct ChunkCoord {
    int32_t cx{0};
    int32_t cz{0};
    bool operator==(const ChunkCoord& o) const { return cx == o.cx && cz == o.cz; }
};

// ── Height sample ─────────────────────────────────────────────────────────
struct HeightSample {
    float       height{0.0f};       ///< World-space y height
    uint8_t     biomeId{0};
    std::string material;           ///< e.g. "grass", "rock", "sand", "snow"
    float       moisture{0.5f};     ///< [0,1] used for biome selection
    float       temperature{0.5f};  ///< [0,1] used for biome selection
};

// ── Decoration ────────────────────────────────────────────────────────────
struct Decoration {
    uint32_t    localX{0};
    uint32_t    localZ{0};
    float       yOffset{0.0f};
    std::string type;         ///< e.g. "tree_pine", "rock_large", "spawn_point"
    float       scale{1.0f};
    float       rotation{0.0f}; ///< Y-axis rotation radians
};

// ── World chunk ───────────────────────────────────────────────────────────
struct WorldChunk {
    ChunkCoord              coord{};
    uint32_t                size{0};    ///< Samples per side (size×size grid)
    std::vector<HeightSample> samples;  ///< Row-major: samples[z*size + x]
    std::vector<Decoration>   decorations;

    const HeightSample& At(uint32_t x, uint32_t z) const { return samples[z * size + x]; }
    HeightSample&       At(uint32_t x, uint32_t z)       { return samples[z * size + x]; }
    bool                Valid() const { return !samples.empty(); }
};

// ── Generator config ──────────────────────────────────────────────────────
struct WorldChunkConfig {
    uint32_t chunkSize{32};         ///< Samples per chunk side
    float    worldScale{64.0f};     ///< World-space units per chunk
    uint32_t noiseOctaves{6};
    float    noiseLacunarity{2.0f};
    float    noisePersistence{0.5f};
    float    heightScale{128.0f};   ///< Max terrain height
    float    seaLevel{0.3f};        ///< [0,1] fraction of heightScale
    uint32_t maxCacheSize{256};     ///< Max chunks held in LRU cache
    uint64_t seed{42};
    bool     placeDecorations{true};
    float    decorationDensity{0.05f}; ///< Decorations per sample (avg)
};

// ── Generator ─────────────────────────────────────────────────────────────
class WorldChunkGen {
public:
    WorldChunkGen();
    explicit WorldChunkGen(const WorldChunkConfig& cfg);
    ~WorldChunkGen();

    WorldChunkGen(const WorldChunkGen&) = delete;
    WorldChunkGen& operator=(const WorldChunkGen&) = delete;

    void SetConfig(const WorldChunkConfig& cfg);
    const WorldChunkConfig& Config() const;

    // ── generation ────────────────────────────────────────────
    /// Generate (or return cached) chunk at (cx, cz).
    const WorldChunk& GenerateChunk(int32_t cx, int32_t cz);

    /// Generate all chunks within Manhattan radius r of (cx, cz).
    std::vector<ChunkCoord> LoadRadius(int32_t cx, int32_t cz, uint32_t radius);

    // ── cache ─────────────────────────────────────────────────
    bool     IsCached(int32_t cx, int32_t cz) const;
    void     EvictChunk(int32_t cx, int32_t cz);
    void     ClearCache();
    size_t   CacheSize() const;

    // ── callbacks ─────────────────────────────────────────────
    using ChunkReadyCb = std::function<void(const WorldChunk&)>;
    void OnChunkReady(ChunkReadyCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace PCG
