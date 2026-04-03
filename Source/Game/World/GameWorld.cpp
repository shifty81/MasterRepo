#include "Game/World/GameWorld.h"
#include "Core/Logging/Log.h"
#include <cmath>
#include <vector>

namespace NF::Game {

bool GameWorld::Initialize(const std::string& contentRoot)
{
    // Load dev world config
    const std::string configPath = contentRoot + "/Definitions/DevWorld.json";
    m_UsedFallbackDefinition = false;
    if (!m_Config.LoadFromFile(configPath)) {
        Logger::Log(LogLevel::Warning, "GameWorld",
                    "DevWorld config not found at " + configPath + "; using defaults");
        m_Config = DevWorldConfig::Defaults();
        m_UsedFallbackDefinition = true;
    }

    Logger::Log(LogLevel::Info, "GameWorld",
                "[World] Definition loaded: " + m_Config.WorldId());
    Logger::Log(LogLevel::Info, "GameWorld",
                std::string("[World] Fallback definition used: ") + (m_UsedFallbackDefinition ? "YES" : "NO"));
    Logger::Log(LogLevel::Info, "GameWorld",
                "[World] Seed: " + std::to_string(m_Config.Seed()));

    // Create a player entity at the spawn point
    m_Level.Load(m_Config.WorldId());
    auto& world = m_Level.GetWorld();
    m_PlayerEntity = world.CreateEntity();
    Logger::Log(LogLevel::Info, "GameWorld",
                "[World] Player entity created: " + std::to_string(m_PlayerEntity));

    // Log spawn point
    const auto& sp = m_Config.GetSpawnPoint();
    Logger::Log(LogLevel::Info, "GameWorld",
                "[World] Spawn initialized: (" + std::to_string(sp.Position.X) + ", "
                + std::to_string(sp.Position.Y) + ", "
                + std::to_string(sp.Position.Z) + ")");

    m_Ready = true;

    // --- Phase 4: generate starter terrain chunks around spawn ---
    {
        // Use floor division to handle negative spawn positions correctly
        const int32_t spawnChunkX = static_cast<int32_t>(std::floor(sp.Position.X / kChunkSize));
        const int32_t spawnChunkY = 0;
        const int32_t spawnChunkZ = static_cast<int32_t>(std::floor(sp.Position.Z / kChunkSize));

        m_InitialGeneratedChunkCount = 0;
        for (int cx = spawnChunkX - 1; cx <= spawnChunkX + 1; ++cx) {
            for (int cz = spawnChunkZ - 1; cz <= spawnChunkZ + 1; ++cz) {
                ChunkCoord coord{cx, spawnChunkY, cz};
                Chunk* chunk = m_ChunkMap.GetOrCreateChunk(coord);

                for (uint8_t x = 0; x < kChunkSize; ++x) {
                    for (uint8_t z = 0; z < kChunkSize; ++z) {
                        const int wx = cx * kChunkSize + x;
                        const int wz = cz * kChunkSize + z;
                        const int height = 8 + (wx * 3 + wz * 7) % 5;

                        for (uint8_t y = 0; y < kChunkSize && y < height; ++y) {
                            VoxelType type;
                            if (y < 3)
                                type = VoxelType::Rock;
                            else if (y < height - 2)
                                type = VoxelType::Stone;
                            else if (y == height - 1)
                                type = VoxelType::Dirt;
                            else
                                type = VoxelType::Stone;

                            if (y > 2 && y < 8 && ((wx + y * 3 + wz * 5) % 17 == 0))
                                type = VoxelType::Ore;

                            chunk->SetVoxel(x, y, z, static_cast<VoxelId>(type));
                        }
                    }
                }

                ++m_InitialGeneratedChunkCount;
            }
        }

        const int loadedChunks = static_cast<int>(m_ChunkMap.ChunkCount());
        Logger::Log(LogLevel::Info, "GameWorld",
                    "[World] Initial chunks generated: " + std::to_string(m_InitialGeneratedChunkCount));
        Logger::Log(LogLevel::Info, "GameWorld",
                    "[World] Chunk map loaded count: " + std::to_string(loadedChunks));

        m_BootstrapStatusText = loadedChunks > 0
            ? "descriptor loaded, chunks populated"
            : "descriptor loaded, chunk map empty";

        Logger::Log(LogLevel::Info, "GameWorld",
                    "[Viewport] State: " + std::string(loadedChunks > 0 ? "POPULATED" : "EMPTY"));
    }

    Logger::Log(LogLevel::Info, "GameWorld", "Initialized");
    return true;
}

void GameWorld::Tick(float dt)
{
    if (!m_Ready) return;
    m_Level.Update(dt);
}

void GameWorld::Shutdown()
{
    if (!m_Ready) return;
    m_Level.Unload();
    m_PlayerEntity = NullEntity;
    m_Ready = false;
    m_BootstrapStatusText = "shutdown";
    Logger::Log(LogLevel::Info, "GameWorld", "Shutdown");
}

bool GameWorld::SaveWorld(const std::string& path)
{
    if (!m_Ready) {
        Logger::Log(LogLevel::Warning, "GameWorld",
                    "Cannot save — world not initialized");
        return false;
    }

    // Collect live entity ids from the ECS world
    // For Phase 1, we save only the player entity as a proof of concept
    std::vector<uint32_t> entityIds;
    if (m_PlayerEntity != NullEntity)
        entityIds.push_back(m_PlayerEntity);

    return m_SaveLoad.SaveToFile(path, m_Config.Seed(), entityIds);
}

bool GameWorld::LoadWorld(const std::string& path)
{
    if (!m_SaveLoad.LoadFromFile(path))
        return false;

    const auto& header = m_SaveLoad.GetHeader();
    Logger::Log(LogLevel::Info, "GameWorld",
                "Loaded save — seed: " + std::to_string(header.Seed)
                + ", entities: " + std::to_string(header.EntityCount));
    return true;
}

void GameWorld::LogDebugOverlay()
{
    m_DebugOverlay.Update(m_Config,
                          m_Ready ? &m_Level.GetWorld() : nullptr,
                          m_PlayerEntity);
    m_DebugOverlay.LogToConsole();
}

void GameWorld::LogVoxelDebug()
{
    VoxelDebugOverlay::ValidateMap(m_ChunkMap);
    VoxelDebugOverlay::LogStats(m_ChunkMap);
}

bool GameWorld::SaveChunks(const std::string& chunkPath)
{
    return VoxelSerializer::SaveMap(m_ChunkMap, chunkPath);
}

bool GameWorld::LoadChunks(const std::string& chunkPath)
{
    return VoxelSerializer::LoadMap(m_ChunkMap, chunkPath);
}

} // namespace NF::Game
