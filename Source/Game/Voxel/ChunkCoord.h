#pragma once
#include <cstdint>
#include <functional>

namespace NF::Game {

/// @brief Chunk size in voxels along each axis.
///
/// All chunks are cubic — this single constant defines the edge length.
/// Locked at 32 per the voxel-first directives.
inline constexpr int kChunkSize = 32;

/// @brief Integer chunk-grid coordinate (one unit = one chunk).
struct ChunkCoord {
    int32_t X{0};
    int32_t Y{0};
    int32_t Z{0};

    [[nodiscard]] bool operator==(const ChunkCoord& o) const noexcept {
        return X == o.X && Y == o.Y && Z == o.Z;
    }
    [[nodiscard]] bool operator!=(const ChunkCoord& o) const noexcept {
        return !(*this == o);
    }
};

/// @brief 3-D integer position in local voxel space within a chunk [0, kChunkSize).
struct LocalVoxelCoord {
    uint8_t X{0};
    uint8_t Y{0};
    uint8_t Z{0};
};

/// @brief Convert a world-space voxel position to the chunk that contains it.
[[nodiscard]] inline ChunkCoord WorldToChunk(int32_t wx, int32_t wy, int32_t wz) noexcept {
    // Use arithmetic (floor) division so negative world coords map correctly.
    auto floorDiv = [](int32_t a, int32_t b) -> int32_t {
        return a / b - (a % b != 0 && (a ^ b) < 0);
    };
    return { floorDiv(wx, kChunkSize),
             floorDiv(wy, kChunkSize),
             floorDiv(wz, kChunkSize) };
}

/// @brief Convert a world-space voxel position to local coords within its chunk.
[[nodiscard]] inline LocalVoxelCoord WorldToLocal(int32_t wx, int32_t wy, int32_t wz) noexcept {
    auto mod = [](int32_t a, int32_t b) -> uint8_t {
        int32_t r = a % b;
        if (r < 0) r += b;
        return static_cast<uint8_t>(r);
    };
    return { mod(wx, kChunkSize), mod(wy, kChunkSize), mod(wz, kChunkSize) };
}

/// @brief Compute the world-space origin (minimum corner) of a chunk.
inline void ChunkOrigin(const ChunkCoord& c,
                        int32_t& ox, int32_t& oy, int32_t& oz) noexcept {
    ox = c.X * kChunkSize;
    oy = c.Y * kChunkSize;
    oz = c.Z * kChunkSize;
}

/// @brief Convert a local voxel coord to a flat 1-D index within the chunk.
/// @pre X, Y, Z are all in [0, kChunkSize).
[[nodiscard]] inline int32_t LocalToIndex(uint8_t x, uint8_t y, uint8_t z) noexcept {
    return (static_cast<int32_t>(z) * kChunkSize + static_cast<int32_t>(y)) * kChunkSize
           + static_cast<int32_t>(x);
}

/// @brief Total number of voxels in a chunk.
inline constexpr int32_t kChunkVolume = kChunkSize * kChunkSize * kChunkSize;

} // namespace NF::Game

// ---------------------------------------------------------------------------
// std::hash specialisation so ChunkCoord can be used in unordered_map.
// ---------------------------------------------------------------------------
namespace std {

template<>
struct hash<NF::Game::ChunkCoord> {
    std::size_t operator()(const NF::Game::ChunkCoord& c) const noexcept {
        // Combine three int32_t fields with prime multipliers.
        std::size_t h = static_cast<std::size_t>(c.X) * 73856093u;
        h ^= static_cast<std::size_t>(c.Y) * 19349663u;
        h ^= static_cast<std::size_t>(c.Z) * 83492791u;
        return h;
    }
};

} // namespace std
