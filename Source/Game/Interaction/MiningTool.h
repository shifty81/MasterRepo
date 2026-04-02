#pragma once
#include "Game/Voxel/VoxelEditApi.h"
#include "Game/Interaction/ResourceItem.h"
#include <cstdint>

namespace NF::Game {

class RigState;

/// @brief Combined result of a single mining-tool swing.
struct MiningResult {
    MineReport    voxelReport; ///< Underlying voxel operation result.
    ResourceStack gathered;    ///< Resource collected (IsEmpty() if none).
};

/// @brief Starter interaction tool: a basic mining/extraction device.
///
/// On each use the tool:
///   1. Attempts to mine the target voxel via VoxelEditApi.
///   2. On success, converts the destroyed voxel into a ResourceStack.
///   3. Deducts a fixed energy cost from the supplied RigState.
///
/// Energy cost is deducted even when the voxel survives (partial damage).
class MiningTool {
public:
    /// @brief Energy consumed from the rig per swing.
    static constexpr float    kEnergyCostPerSwing = 5.f;

    /// @brief Resource units yielded per successfully mined voxel.
    static constexpr uint32_t kYieldPerVoxel = 1;

    MiningTool() = default;

    /// @brief Attempt to mine the voxel at (wx, wy, wz).
    ///
    /// @param api      Active VoxelEditApi providing world access.
    /// @param rig      The player rig (energy is consumed).
    /// @param wx,wy,wz World-space integer voxel coordinates.
    /// @return         A MiningResult; gathered.IsEmpty() when nothing was extracted.
    MiningResult Use(VoxelEditApi& api, RigState& rig,
                     int32_t wx, int32_t wy, int32_t wz);
};

} // namespace NF::Game
