#pragma once
#include "Game/Voxel/ChunkCoord.h"
#include "Game/Voxel/VoxelType.h"
#include <array>
#include <cstdint>

namespace NF::Game {

/// @brief A 32 × 32 × 32 voxel chunk.
///
/// Stores one @c VoxelId per voxel in a flat array.  Provides
/// bounds-checked get/set in debug builds and unchecked variants
/// for performance-critical inner loops.
///
/// Dirty state tracks whether the chunk has been modified since
/// the last serialization / mesh rebuild.
class Chunk {
public:
    // ---- Construction -------------------------------------------------------

    explicit Chunk(const ChunkCoord& coord) noexcept : m_Coord(coord) {
        m_Voxels.fill(static_cast<VoxelId>(VoxelType::Air));
    }

    // ---- Identity -----------------------------------------------------------

    [[nodiscard]] const ChunkCoord& GetCoord() const noexcept { return m_Coord; }

    // ---- Voxel access -------------------------------------------------------

    /// @brief Return the voxel at local coords — bounds-checked in debug.
    /// @pre x, y, z ∈ [0, kChunkSize)
    [[nodiscard]] VoxelId GetVoxel(uint8_t x, uint8_t y, uint8_t z) const noexcept {
#ifdef NDEBUG
        return m_Voxels[LocalToIndex(x, y, z)];
#else
        if (x >= kChunkSize || y >= kChunkSize || z >= kChunkSize)
            return static_cast<VoxelId>(VoxelType::Air);
        return m_Voxels[LocalToIndex(x, y, z)];
#endif
    }

    /// @brief Set a voxel at local coords and mark the chunk dirty.
    /// @pre x, y, z ∈ [0, kChunkSize)
    /// @return false if coordinates are out of range.
    bool SetVoxel(uint8_t x, uint8_t y, uint8_t z, VoxelId id) noexcept {
        if (x >= kChunkSize || y >= kChunkSize || z >= kChunkSize)
            return false;
        m_Voxels[LocalToIndex(x, y, z)] = id;
        m_Dirty = true;
        return true;
    }

    /// @brief Return the flat voxel data (read-only).
    [[nodiscard]] const std::array<VoxelId, kChunkVolume>& GetRawData() const noexcept {
        return m_Voxels;
    }

    // ---- Utility ------------------------------------------------------------

    /// @brief Return true if all voxels in the chunk are Air.
    [[nodiscard]] bool IsEmpty() const noexcept {
        for (auto v : m_Voxels)
            if (v != static_cast<VoxelId>(VoxelType::Air)) return false;
        return true;
    }

    /// @brief Count non-air voxels.
    [[nodiscard]] int32_t CountSolid() const noexcept {
        int32_t count = 0;
        for (auto v : m_Voxels)
            if (v != static_cast<VoxelId>(VoxelType::Air)) ++count;
        return count;
    }

    // ---- Dirty state --------------------------------------------------------

    [[nodiscard]] bool IsDirty() const noexcept { return m_Dirty; }

    /// @brief Clear the dirty flag (call after serialization / meshing).
    void ClearDirty() noexcept { m_Dirty = false; }

    /// @brief Force-mark dirty (used by edit API).
    void MarkDirty() noexcept { m_Dirty = true; }

private:
    ChunkCoord                         m_Coord;
    std::array<VoxelId, kChunkVolume>  m_Voxels{};
    bool                               m_Dirty{false};
};

} // namespace NF::Game
