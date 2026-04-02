#include "Game/App/Orchestrator.h"
#include "Game/World/DevWorldConfig.h"
#include "Core/Logging/Log.h"

namespace NF::Game {

bool Orchestrator::Init(RenderDevice* renderDevice)
{
    m_RenderDevice = renderDevice;

    // Load the dev world via GameWorld (voxel layer + ECS + dev config).
    m_GameWorld.Initialize("Content");

    // Load the engine level alongside the game world.
    m_Level.Load("DevWorld");

    // Wire the Phase 3 interaction loop to the voxel edit API.
    m_InteractionLoop.Init(&m_GameWorld.GetVoxelEditApi());

    // Phase 5: Spawn the player movement controller at the world spawn point.
    {
        const auto& sp = m_GameWorld.GetSpawnPoint();
        m_PlayerMovement.SetPosition({sp.Position.X,
                                       sp.Position.Y + 2.f, // start slightly above terrain
                                       sp.Position.Z});
    }

    m_Initialized = true;
    Logger::Log(LogLevel::Info, "Game", "Orchestrator::Init complete");
    return true;
}

void Orchestrator::Tick(float dt)
{
    if (!m_Initialized) return;

    m_Level.Update(dt);
    m_GameWorld.Tick(dt);
    m_InteractionLoop.Tick(dt);

    // Phase 5: integrate player movement with voxel collision.
    m_PlayerMovement.Update(dt, m_GameWorld.GetChunkMap());

    if (m_RenderDevice)
    {
        m_RenderDevice->BeginFrame();
        m_RenderDevice->Clear(0.05f, 0.05f, 0.08f, 1.0f);
        m_RenderDevice->EndFrame();
    }
}

void Orchestrator::Shutdown()
{
    if (!m_Initialized) return;

    m_GameWorld.Shutdown();
    m_Level.Unload();
    m_RenderDevice = nullptr;
    m_Initialized  = false;
    Logger::Log(LogLevel::Info, "Game", "Orchestrator::Shutdown");
}

} // namespace NF::Game
