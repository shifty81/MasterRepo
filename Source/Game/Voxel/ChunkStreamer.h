#pragma once
#include "Game/Voxel/ChunkMap.h"
#include "Game/Voxel/ChunkMeshCache.h"
#include "Game/Voxel/VoxelSerializer.h"
#include "Core/Threading/TaskSystem.h"
#include <mutex>
#include <vector>
#include <unordered_set>
#include <functional>

namespace NF::Game {

/// @brief Configuration for the chunk streaming system.
struct ChunkStreamConfig {
    int      LoadRadius{4};          ///< Chunk radius around the viewer to keep loaded.
    int      UnloadRadius{6};        ///< Chunk radius beyond which chunks are evicted.
    size_t   MaxLoadedChunks{512};   ///< Hard cap on loaded chunks (memory budget).
    int      MaxLoadsPerTick{4};     ///< Maximum chunks to generate per frame.
    int      MaxUnloadsPerTick{4};   ///< Maximum chunks to unload per frame.
    bool     SaveOnUnload{true};     ///< Serialize modified chunks before eviction.
    std::string SaveDirectory;       ///< Directory for per-chunk save files.
};

/// @brief LOD level for chunk mesh generation.
enum class ChunkLOD : uint8_t {
    Full       = 0,  ///< Full detail — every face emitted.
    Simplified = 1,  ///< Reduced detail — skip interior faces.
    None       = 2,  ///< No mesh — too far from the viewer.
};

/// @brief Streams chunks in and out of memory based on viewer position.
///
/// Maintains a loaded set of chunks within a configurable radius around the
/// viewer.  Chunks beyond the unload radius are evicted (optionally saved
/// if dirty).  Background-thread terrain generation uses the existing
/// @c TaskSystem for async work.
///
/// Call @c Tick() every frame from the Orchestrator after updating the
/// player position.
class ChunkStreamer {
public:
    /// @brief Terrain generator callback: fills a chunk's voxels given its
    ///        coordinate and the world seed.
    using GeneratorFn = std::function<void(Chunk& chunk, uint32_t seed)>;

    ChunkStreamer() = default;

    // ---- Lifecycle ----------------------------------------------------------

    /// @brief Initialise the streamer.
    /// @param map           Non-owning pointer to the authoritative ChunkMap.
    /// @param meshCache     Non-owning pointer to the mesh cache (may be null
    ///                      for headless / dedicated).
    /// @param config        Streaming configuration.
    /// @param worldSeed     Seed for procedural generation.
    /// @param generator     Terrain generation callback (optional; defaults to
    ///                      the built-in terrain generator).
    void Init(ChunkMap* map,
              ChunkMeshCache* meshCache,
              const ChunkStreamConfig& config,
              uint32_t worldSeed,
              GeneratorFn generator = nullptr);

    /// @brief Shut down the streamer and join any in-flight tasks.
    void Shutdown();

    // ---- Per-frame ----------------------------------------------------------

    /// @brief Update streaming based on the viewer's chunk coordinate.
    ///
    /// 1. Determine which chunks should be loaded (within load radius).
    /// 2. Queue generation for missing chunks (up to MaxLoadsPerTick).
    /// 3. Receive completed chunks from background tasks.
    /// 4. Evict chunks beyond unload radius (up to MaxUnloadsPerTick).
    /// 5. Enforce memory budget via LRU eviction if over limit.
    /// @param viewerChunk  Chunk coordinate of the viewer / player.
    void Tick(const ChunkCoord& viewerChunk);

    // ---- Queries ------------------------------------------------------------

    /// @brief Number of chunks currently pending background generation.
    [[nodiscard]] int PendingCount() const noexcept;

    /// @brief Number of chunks evicted since Init().
    [[nodiscard]] int TotalEvicted() const noexcept { return m_TotalEvicted; }

    /// @brief Number of chunks generated since Init().
    [[nodiscard]] int TotalGenerated() const noexcept { return m_TotalGenerated; }

    /// @brief Current configuration.
    [[nodiscard]] const ChunkStreamConfig& GetConfig() const noexcept { return m_Config; }

    /// @brief Compute the LOD level for a chunk at the given distance.
    [[nodiscard]] static ChunkLOD ComputeLOD(int distSq, int loadRadiusSq);

    // ---- Accessors ----------------------------------------------------------

    [[nodiscard]] bool IsInitialized() const noexcept { return m_Initialized; }

    /// @brief Built-in terrain generator (matches GameWorld::Initialize pattern).
    static void DefaultGenerator(Chunk& chunk, uint32_t seed);

    /// @brief Squared distance between two chunk coords.
    [[nodiscard]] static int DistanceSq(const ChunkCoord& a, const ChunkCoord& b) noexcept;

private:
    // ---- Internal helpers ---------------------------------------------------

    /// @brief Save a dirty chunk to disk.
    void SaveChunkToDisk(const Chunk& chunk);

    // ---- State --------------------------------------------------------------

    ChunkMap*          m_Map{nullptr};
    ChunkMeshCache*    m_MeshCache{nullptr};
    ChunkStreamConfig  m_Config;
    uint32_t           m_WorldSeed{0};
    GeneratorFn        m_Generator;
    bool               m_Initialized{false};

    int  m_TotalEvicted{0};
    int  m_TotalGenerated{0};

    // Background generation
    NF::TaskSystem  m_TaskSystem;

    /// @brief A completed chunk ready to be inserted into the map.
    struct PendingChunk {
        ChunkCoord               Coord;
        std::unique_ptr<Chunk>   Data;
    };

    std::mutex                  m_PendingMutex;
    std::vector<PendingChunk>   m_PendingResults;      ///< Completed by workers.
    std::unordered_set<ChunkCoord> m_InFlight;         ///< Coords being generated.
    std::vector<std::future<void>> m_Futures;          ///< Worker futures.

    ChunkCoord  m_LastViewerChunk{0, 0, 0};
};

} // namespace NF::Game

// Hash for unordered_set<ChunkCoord> — already provided by ChunkCoord.h
