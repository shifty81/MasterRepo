#include "Game/World/GameWorld.h"
#include "Core/Logging/Log.h"
#include <vector>

namespace NF::Game {

bool GameWorld::Initialize(const std::string& contentRoot)
{
    // Load dev world config
    const std::string configPath = contentRoot + "/Definitions/DevWorld.json";
    if (!m_Config.LoadFromFile(configPath))
    {
        Logger::Log(LogLevel::Warning, "GameWorld",
                    "DevWorld config not found at " + configPath + "; using defaults");
        m_Config = DevWorldConfig::Defaults();
    }

    // Load the level using the world id
    m_Level.Load(m_Config.WorldId());

    // Log world identity and seed at startup
    Logger::Log(LogLevel::Info, "GameWorld",
                "World: " + m_Config.WorldId()
                + " | Seed: " + std::to_string(m_Config.Seed()));

    // Create a player entity at the spawn point
    auto& world = m_Level.GetWorld();
    m_PlayerEntity = world.CreateEntity();
    Logger::Log(LogLevel::Info, "GameWorld",
                "Player entity created: " + std::to_string(m_PlayerEntity));

    // Log spawn point
    const auto& sp = m_Config.GetSpawnPoint();
    Logger::Log(LogLevel::Info, "GameWorld",
                "Spawn point: (" + std::to_string(sp.Position.X) + ", "
                + std::to_string(sp.Position.Y) + ", "
                + std::to_string(sp.Position.Z) + ")");

    m_Ready = true;
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
