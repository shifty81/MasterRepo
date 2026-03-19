#pragma once
#include "TileRenderer.h"
#include "../../editor/tools/TileEditorModule.h"
#include <cstdint>
#include <set>

namespace atlas::tile {

/// Coordinate of a chunk in chunk-grid space.
struct ChunkCoord {
    int32_t cx = 0;
    int32_t cy = 0;

    bool operator==(const ChunkCoord& o) const { return cx == o.cx && cy == o.cy; }
    bool operator<(const ChunkCoord& o) const {
        if (cx != o.cx) return cx < o.cx;
        return cy < o.cy;
    }
};

/// Builds GPU-ready tile chunk meshes from a TileMap + TileLayer.
///
/// Determinism guarantees:
///  * Fixed iteration order (sorted coordinates).
///  * No floating-point randomness.
///  * Atlas UVs baked once per tile asset.
///  * Identical inputs on any platform produce identical output.
class TileChunkBuilder {
public:
    /// Number of grid cells per chunk side.
    static constexpr int32_t kChunkSize = 8;

    /// Build mesh data for all tiles in @p layer that fall within the
    /// chunk starting at @p chunkOrigin.
    static void Build(
        const atlas::editor::TileMap& map,
        const atlas::editor::TileLayer& layer,
        const ChunkCoord& chunkOrigin,
        TileChunk& outChunk);

    /// Convert a world grid coordinate to the chunk it belongs to.
    static ChunkCoord WorldToChunk(const atlas::editor::GridCoord& coord);

    /// Check whether a grid coordinate is inside a chunk.
    static bool IsInsideChunk(const atlas::editor::GridCoord& coord,
                              const ChunkCoord& chunkOrigin);

    /// Mark a chunk as dirty after a tile change.
    static void MarkDirty(std::set<ChunkCoord>& dirtySet,
                          const atlas::editor::GridCoord& coord);
};

} // namespace atlas::tile
