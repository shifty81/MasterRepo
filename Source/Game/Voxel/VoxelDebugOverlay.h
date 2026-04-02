#pragma once
#include "Game/Voxel/ChunkMap.h"
#include <string>

namespace NF::Game {

/// @brief Runtime validation and diagnostic logging for the voxel layer.
///
/// Checks chunk integrity and logs summary statistics to the NovaForge logger.
/// Call from debug/test builds to catch data corruption early.
class VoxelDebugOverlay {
public:
    // ---- Validation ---------------------------------------------------------

    /// @brief Validate every loaded chunk in @p map.
    ///
    /// Checks that no chunk stores an undefined voxel ID and that the chunk's
    /// reported dirty state is internally consistent.
    ///
    /// @return True if all chunks pass validation.
    static bool ValidateMap(const ChunkMap& map);

    /// @brief Validate a single chunk.
    /// @return True if the chunk passes all integrity checks.
    static bool ValidateChunk(const Chunk& chunk);

    // ---- Statistics ---------------------------------------------------------

    /// @brief Log a summary of all loaded chunks to the logger.
    static void LogStats(const ChunkMap& map);

    /// @brief Return a formatted summary string for display in editor panels.
    [[nodiscard]] static std::string BuildStatsString(const ChunkMap& map);
};

} // namespace NF::Game
