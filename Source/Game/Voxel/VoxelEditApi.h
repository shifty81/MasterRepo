#pragma once
#include "Game/Voxel/ChunkMap.h"
#include "Game/Voxel/VoxelType.h"
#include <cstdint>

namespace NF::Game {

/// @brief Result of a mining operation.
enum class MineResult {
    Success,        ///< Voxel was removed.
    OutOfBounds,    ///< World position was invalid.
    NotMineable,    ///< Voxel type cannot be mined (e.g. Air).
    DurabilityLeft, ///< Voxel was damaged but not destroyed.
};

/// @brief Result of a voxel-set operation.
enum class SetResult {
    Success,    ///< Voxel was written.
    OutOfBounds ///< Position was invalid (should never happen with a ChunkMap).
};

/// @brief Immutable result returned from Mine() carrying the extracted type.
struct MineReport {
    MineResult result{MineResult::OutOfBounds};
    VoxelId    extractedType{static_cast<VoxelId>(VoxelType::Air)};
    uint8_t    remainingDurability{0}; ///< 0 = destroyed, >0 = damage applied.
};

/// @brief High-level voxel edit operations.
///
/// All operations work in world-space integer coordinates.  Chunks are
/// created on demand when a write operation targets an unloaded position.
///
/// Durability for damage/mining is stored externally as a caller-supplied
/// per-voxel value (the API does not own durability state — callers manage it
/// through the game simulation).  For convenience, Mine() treats a voxel as
/// fully durability-intact unless the caller passes a durability argument.
class VoxelEditApi {
public:
    explicit VoxelEditApi(ChunkMap& map) noexcept : m_Map(map) {}

    // ---- Read ---------------------------------------------------------------

    /// @brief Return the voxel at world position (wx, wy, wz).
    /// @return Air if the chunk is not loaded.
    [[nodiscard]] VoxelId GetVoxel(int32_t wx, int32_t wy, int32_t wz) const noexcept;

    // ---- Write --------------------------------------------------------------

    /// @brief Place a voxel at world position, creating the chunk if needed.
    SetResult SetVoxel(int32_t wx, int32_t wy, int32_t wz, VoxelId id);

    /// @brief Convenience wrapper accepting the enum.
    SetResult SetVoxel(int32_t wx, int32_t wy, int32_t wz, VoxelType type) {
        return SetVoxel(wx, wy, wz, static_cast<VoxelId>(type));
    }

    // ---- Damage / Mining ----------------------------------------------------

    /// @brief Remove a voxel (mine it).
    ///
    /// If the voxel is not minable the operation fails without modifying state.
    /// The caller may pass a @p durability value (0 = instant removal;
    /// values > 0 are decremented by the voxel's hardness and the remainder
    /// is reported back — when durability reaches 0 the voxel is destroyed).
    ///
    /// @param wx, wy, wz  World-space voxel position.
    /// @param durability  Current durability of the target voxel (in/out).
    ///                    Pass 0 to mine instantly.
    MineReport Mine(int32_t wx, int32_t wy, int32_t wz,
                    uint8_t durability = 0);

    /// @brief Damage a voxel without destroying it.
    ///
    /// Decrements durability by the voxel's hardness value.
    /// Returns the remaining durability (0 means the voxel was destroyed
    /// and has already been removed from the chunk).
    uint8_t Damage(int32_t wx, int32_t wy, int32_t wz, uint8_t durability);

    /// @brief Repair a voxel back to a specific type (e.g. after repair tool).
    SetResult Repair(int32_t wx, int32_t wy, int32_t wz, VoxelId id);

private:
    ChunkMap& m_Map;
};

} // namespace NF::Game
