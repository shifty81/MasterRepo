#include "Game/Voxel/VoxelDebugOverlay.h"
#include "Game/Voxel/VoxelType.h"
#include "Core/Logging/Log.h"
#include <sstream>
#include <string>

namespace NF::Game {

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

bool VoxelDebugOverlay::ValidateChunk(const Chunk& chunk)
{
    const auto& raw = chunk.GetRawData();
    constexpr VoxelId kMaxKnownId = static_cast<VoxelId>(VoxelType::Organic);

    bool ok = true;
    int32_t badVoxels = 0;

    for (auto v : raw) {
        if (v > kMaxKnownId) {
            ++badVoxels;
            ok = false;
        }
    }

    if (!ok) {
        const auto& c = chunk.GetCoord();
        Logger::Log(LogLevel::Warning, "VoxelDebug",
                    "Chunk (" + std::to_string(c.X) + ","
                    + std::to_string(c.Y) + ","
                    + std::to_string(c.Z) + "): "
                    + std::to_string(badVoxels)
                    + " unrecognised voxel IDs");
    }

    return ok;
}

bool VoxelDebugOverlay::ValidateMap(const ChunkMap& map)
{
    bool allOk = true;
    int32_t total    = 0;
    int32_t failures = 0;

    const auto& coords = map.GetLoadedCoords();
    for (const auto& coord : coords) {
        const Chunk* chunk = map.GetChunk(coord);
        if (!chunk) continue;
        ++total;
        if (!ValidateChunk(*chunk)) {
            ++failures;
            allOk = false;
        }
    }

    Logger::Log(allOk ? LogLevel::Info : LogLevel::Warning,
                "VoxelDebug",
                "Validation: " + std::to_string(total - failures)
                + "/" + std::to_string(total) + " chunks OK");

    return allOk;
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

std::string VoxelDebugOverlay::BuildStatsString(const ChunkMap& map)
{
    size_t chunkCount  = map.ChunkCount();
    int64_t solidTotal = 0;
    int32_t dirtyCount = 0;
    const auto& coords = map.GetLoadedCoords();

    for (const auto& coord : coords) {
        const Chunk* chunk = map.GetChunk(coord);
        if (!chunk) continue;
        solidTotal += chunk->CountSolid();
        if (chunk->IsDirty()) ++dirtyCount;
    }

    std::ostringstream oss;
    oss << "Chunks: "     << chunkCount
        << "  Solid: "    << solidTotal
        << "  Dirty: "    << dirtyCount;
    return oss.str();
}

void VoxelDebugOverlay::LogStats(const ChunkMap& map)
{
    Logger::Log(LogLevel::Info, "VoxelDebug", BuildStatsString(map));
}

} // namespace NF::Game
