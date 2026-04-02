#include "Game/Voxel/ChunkStreamer.h"
#include "Core/Logging/Log.h"
#include <algorithm>
#include <cmath>
#include <fstream>

namespace NF::Game {

// ---------------------------------------------------------------------------
// Init / Shutdown
// ---------------------------------------------------------------------------

void ChunkStreamer::Init(ChunkMap* map,
                         ChunkMeshCache* meshCache,
                         const ChunkStreamConfig& config,
                         uint32_t worldSeed,
                         GeneratorFn generator)
{
    m_Map        = map;
    m_MeshCache  = meshCache;
    m_Config     = config;
    m_WorldSeed  = worldSeed;
    m_Generator  = generator ? std::move(generator)
                             : GeneratorFn(&ChunkStreamer::DefaultGenerator);

    m_TotalEvicted   = 0;
    m_TotalGenerated = 0;
    m_InFlight.clear();
    m_Futures.clear();

    // Start the background thread pool (2 workers for chunk gen).
    m_TaskSystem.Init(2);

    m_Initialized = true;
    NF::Logger::Log(NF::LogLevel::Info, "ChunkStreamer",
                    "Init — loadR=" + std::to_string(config.LoadRadius)
                    + " unloadR=" + std::to_string(config.UnloadRadius)
                    + " budget=" + std::to_string(config.MaxLoadedChunks));
}

void ChunkStreamer::Shutdown()
{
    if (!m_Initialized) return;

    // Wait for all in-flight tasks.
    for (auto& f : m_Futures) {
        if (f.valid()) f.wait();
    }
    m_Futures.clear();
    m_InFlight.clear();

    m_TaskSystem.Shutdown();
    m_Initialized = false;
    NF::Logger::Log(NF::LogLevel::Info, "ChunkStreamer", "Shutdown");
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void ChunkStreamer::Tick(const ChunkCoord& viewerChunk)
{
    if (!m_Initialized || !m_Map) return;

    m_LastViewerChunk = viewerChunk;

    // ------------------------------------------------------------------
    // 1. Receive completed chunks from background tasks.
    // ------------------------------------------------------------------
    {
        std::lock_guard<std::mutex> lock(m_PendingMutex);
        for (auto& pc : m_PendingResults)
        {
            if (!m_Map->HasChunk(pc.Coord))
            {
                // Insert the generated chunk into the map.
                Chunk* dest = m_Map->CreateChunk(pc.Coord);
                // Copy voxel data from the generated chunk.
                for (uint8_t x = 0; x < kChunkSize; ++x)
                    for (uint8_t y = 0; y < kChunkSize; ++y)
                        for (uint8_t z = 0; z < kChunkSize; ++z)
                            dest->SetVoxel(x, y, z, pc.Data->GetVoxel(x, y, z));

                ++m_TotalGenerated;
            }
            m_InFlight.erase(pc.Coord);
        }
        m_PendingResults.clear();
    }

    // Clean up completed futures.
    m_Futures.erase(
        std::remove_if(m_Futures.begin(), m_Futures.end(),
                        [](const std::future<void>& f) {
                            return f.wait_for(std::chrono::seconds(0))
                                   == std::future_status::ready;
                        }),
        m_Futures.end());

    // ------------------------------------------------------------------
    // 2. Determine which chunks should be loaded (within load radius).
    //    Queue generation for missing ones.
    // ------------------------------------------------------------------
    int loadsQueued = 0;

    for (int dx = -m_Config.LoadRadius; dx <= m_Config.LoadRadius && loadsQueued < m_Config.MaxLoadsPerTick; ++dx)
    {
        for (int dy = -1; dy <= 1 && loadsQueued < m_Config.MaxLoadsPerTick; ++dy) // Y extent is small for terrain
        {
            for (int dz = -m_Config.LoadRadius; dz <= m_Config.LoadRadius && loadsQueued < m_Config.MaxLoadsPerTick; ++dz)
            {
                const int distSq = dx * dx + dy * dy + dz * dz;
                if (distSq > m_Config.LoadRadius * m_Config.LoadRadius)
                    continue;

                ChunkCoord target{viewerChunk.X + dx,
                                  viewerChunk.Y + dy,
                                  viewerChunk.Z + dz};

                if (m_Map->HasChunk(target))  continue;
                if (m_InFlight.count(target))  continue;

                // Queue background generation.
                m_InFlight.insert(target);

                uint32_t seed = m_WorldSeed;
                GeneratorFn gen = m_Generator;

                auto fut = m_TaskSystem.Submit([this, target, seed, gen]() {
                    auto chunk = std::make_unique<Chunk>(target);
                    gen(*chunk, seed);
                    chunk->MarkDirty(); // needs meshing

                    std::lock_guard<std::mutex> lock(m_PendingMutex);
                    m_PendingResults.push_back({target, std::move(chunk)});
                });
                m_Futures.push_back(std::move(fut));
                ++loadsQueued;
            }
        }
    }

    // ------------------------------------------------------------------
    // 3. Evict chunks beyond unload radius (up to MaxUnloadsPerTick).
    // ------------------------------------------------------------------
    int unloaded = 0;
    const int unloadRadiusSq = m_Config.UnloadRadius * m_Config.UnloadRadius;

    auto coords = m_Map->GetLoadedCoords();
    // Sort by distance from viewer (farthest first for eviction priority).
    std::sort(coords.begin(), coords.end(),
              [&](const ChunkCoord& a, const ChunkCoord& b) {
                  return DistanceSq(a, viewerChunk) > DistanceSq(b, viewerChunk);
              });

    for (const auto& coord : coords)
    {
        if (unloaded >= m_Config.MaxUnloadsPerTick) break;

        const int distSq = DistanceSq(coord, viewerChunk);
        if (distSq <= unloadRadiusSq) break; // sorted; rest are closer

        // Save dirty chunk before eviction.
        if (m_Config.SaveOnUnload)
        {
            Chunk* chunk = m_Map->GetChunk(coord);
            if (chunk && chunk->IsDirty())
                SaveChunkToDisk(*chunk);
        }

        // Evict GPU mesh.
        if (m_MeshCache) m_MeshCache->Evict(coord);

        m_Map->UnloadChunk(coord);
        ++unloaded;
        ++m_TotalEvicted;
    }

    // ------------------------------------------------------------------
    // 4. Enforce memory budget — LRU eviction of farthest chunks.
    // ------------------------------------------------------------------
    if (m_Map->ChunkCount() > m_Config.MaxLoadedChunks)
    {
        // Re-fetch coords after unload pass.
        coords = m_Map->GetLoadedCoords();
        std::sort(coords.begin(), coords.end(),
                  [&](const ChunkCoord& a, const ChunkCoord& b) {
                      return DistanceSq(a, viewerChunk) > DistanceSq(b, viewerChunk);
                  });

        for (const auto& coord : coords)
        {
            if (m_Map->ChunkCount() <= m_Config.MaxLoadedChunks) break;

            if (m_Config.SaveOnUnload)
            {
                Chunk* chunk = m_Map->GetChunk(coord);
                if (chunk && chunk->IsDirty())
                    SaveChunkToDisk(*chunk);
            }

            if (m_MeshCache) m_MeshCache->Evict(coord);
            m_Map->UnloadChunk(coord);
            ++m_TotalEvicted;
        }
    }
}

// ---------------------------------------------------------------------------
// PendingCount
// ---------------------------------------------------------------------------

int ChunkStreamer::PendingCount() const noexcept
{
    return static_cast<int>(m_InFlight.size());
}

// ---------------------------------------------------------------------------
// ComputeLOD
// ---------------------------------------------------------------------------

ChunkLOD ChunkStreamer::ComputeLOD(int distSq, int loadRadiusSq)
{
    if (distSq <= loadRadiusSq / 4)
        return ChunkLOD::Full;
    if (distSq <= loadRadiusSq)
        return ChunkLOD::Simplified;
    return ChunkLOD::None;
}

// ---------------------------------------------------------------------------
// DistanceSq
// ---------------------------------------------------------------------------

int ChunkStreamer::DistanceSq(const ChunkCoord& a, const ChunkCoord& b) noexcept
{
    const int dx = a.X - b.X;
    const int dy = a.Y - b.Y;
    const int dz = a.Z - b.Z;
    return dx * dx + dy * dy + dz * dz;
}

// ---------------------------------------------------------------------------
// DefaultGenerator
// ---------------------------------------------------------------------------

void ChunkStreamer::DefaultGenerator(Chunk& chunk, uint32_t seed)
{
    const auto& coord = chunk.GetCoord();

    // Only Y=0 layer gets terrain; Y<0 = solid rock, Y>0 = air.
    if (coord.Y < 0)
    {
        for (uint8_t x = 0; x < kChunkSize; ++x)
            for (uint8_t y = 0; y < kChunkSize; ++y)
                for (uint8_t z = 0; z < kChunkSize; ++z)
                    chunk.SetVoxel(x, y, z, static_cast<VoxelId>(VoxelType::Rock));
        return;
    }

    if (coord.Y > 0) return; // Air chunk — leave empty.

    // Y == 0: surface terrain (matches GameWorld::Initialize pattern).
    for (uint8_t x = 0; x < kChunkSize; ++x)
    {
        for (uint8_t z = 0; z < kChunkSize; ++z)
        {
            const int wx = coord.X * kChunkSize + x;
            const int wz = coord.Z * kChunkSize + z;

            // Deterministic height based on seed + world position.
            const unsigned uwx = static_cast<unsigned>(wx);
            const unsigned uwz = static_cast<unsigned>(wz);
            const int rawH = static_cast<int>(
                (uwx * 3u + uwz * 7u + static_cast<unsigned>(seed)) % 5u);
            const int height = 8 + rawH;

            for (uint8_t y = 0; y < kChunkSize && y < height; ++y)
            {
                VoxelType type;
                if (y < 3)
                    type = VoxelType::Rock;
                else if (y < height - 2)
                    type = VoxelType::Stone;
                else if (y == height - 1)
                    type = VoxelType::Dirt;
                else
                    type = VoxelType::Stone;

                // Scatter ore veins.
                if (y > 2 && y < 8 &&
                    ((wx + y * 3 + wz * 5 + static_cast<int>(seed)) % 17 == 0))
                {
                    type = VoxelType::Ore;
                }

                chunk.SetVoxel(x, y, z, static_cast<VoxelId>(type));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// SaveChunkToDisk
// ---------------------------------------------------------------------------

void ChunkStreamer::SaveChunkToDisk(const Chunk& chunk)
{
    if (m_Config.SaveDirectory.empty()) return;

    const auto& c = chunk.GetCoord();
    const std::string filename = m_Config.SaveDirectory + "/chunk_"
        + std::to_string(c.X) + "_"
        + std::to_string(c.Y) + "_"
        + std::to_string(c.Z) + ".nfck";

    auto data = VoxelSerializer::SerializeChunk(chunk);

    // Write to file.
    std::ofstream ofs(filename, std::ios::binary);
    if (ofs)
    {
        ofs.write(reinterpret_cast<const char*>(data.data()),
                  static_cast<std::streamsize>(data.size()));
    }
    else
    {
        NF::Logger::Log(NF::LogLevel::Warning, "ChunkStreamer",
                        "Failed to save chunk at " + filename);
    }
}

} // namespace NF::Game
