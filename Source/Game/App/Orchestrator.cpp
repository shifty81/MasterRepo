#include "Game/App/Orchestrator.h"
#include "Game/World/DevWorldConfig.h"
#include "Core/Logging/Log.h"

namespace NF::Game {

// ---------------------------------------------------------------------------
// Backward-compatible Init (Solo mode)
// ---------------------------------------------------------------------------

bool Orchestrator::Init(RenderDevice* renderDevice)
{
    NetParams solo;
    solo.Mode = NetMode::Solo;
    solo.PlayerName = "Player";
    return Init(renderDevice, solo);
}

// ---------------------------------------------------------------------------
// Init with explicit net mode
// ---------------------------------------------------------------------------

bool Orchestrator::Init(RenderDevice* renderDevice, const NetParams& params)
{
    m_RenderDevice = renderDevice;
    m_NetMode      = params.Mode;
    m_NetParams    = params;

    // ---- World + Level (always needed except pure Client) ----

    if (m_NetMode != NetMode::Client)
    {
        m_GameWorld.Initialize("Content");
        m_Level.Load("DevWorld");
        m_InteractionLoop.Init(&m_GameWorld.GetVoxelEditApi());
    }

    // ---- Networking ----

    switch (m_NetMode)
    {
    case NetMode::Solo:
    {
        m_Server = std::make_unique<GameServer>();
        if (!m_Server->Init(&m_GameWorld, 0)) return false;
        m_LocalClientId = m_Server->AddLocalClient(params.PlayerName);

        // Spawn player at world spawn point.
        const auto& sp = m_GameWorld.GetSpawnPoint();
        m_PlayerMovement.SetPosition({sp.Position.X,
                                       sp.Position.Y + 2.f,
                                       sp.Position.Z});
        break;
    }
    case NetMode::ListenServer:
    {
        m_Server = std::make_unique<GameServer>();
        if (!m_Server->Init(&m_GameWorld, params.Port)) return false;
        m_LocalClientId = m_Server->AddLocalClient(params.PlayerName);

        // Spawn player at world spawn point.
        const auto& sp = m_GameWorld.GetSpawnPoint();
        m_PlayerMovement.SetPosition({sp.Position.X,
                                       sp.Position.Y + 2.f,
                                       sp.Position.Z});
        break;
    }
    case NetMode::Dedicated:
    {
        m_Server = std::make_unique<GameServer>();
        if (!m_Server->Init(&m_GameWorld, params.Port)) return false;
        break;
    }
    case NetMode::Client:
    {
        m_Client = std::make_unique<GameClient>();
        if (!m_Client->Connect(params.Host, params.Port, params.PlayerName))
            return false;
        break;
    }
    }

    m_Initialized = true;
    NF::Logger::Log(NF::LogLevel::Info, "Game",
                    "Orchestrator::Init complete (NetMode=" +
                    std::to_string(static_cast<int>(m_NetMode)) + ")");
    return true;
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void Orchestrator::Tick(float dt)
{
    if (!m_Initialized) return;

    switch (m_NetMode)
    {
    case NetMode::Solo:
    case NetMode::ListenServer:
    case NetMode::Dedicated:
    {
        // Server tick (accepts connections, processes input, builds snapshot).
        if (m_Server) m_Server->Tick(dt);

        // World + level tick (authoritative on server side).
        m_Level.Update(dt);
        m_GameWorld.Tick(dt);
        m_InteractionLoop.Tick(dt);

        // Solo / ListenServer: local player movement.
        if (m_NetMode != NetMode::Dedicated)
        {
            m_PlayerMovement.Update(dt, m_GameWorld.GetChunkMap());
        }
        break;
    }
    case NetMode::Client:
    {
        if (m_Client) m_Client->Update(dt);
        break;
    }
    }

    // Render.
    if (m_RenderDevice)
    {
        m_RenderDevice->BeginFrame();
        m_RenderDevice->Clear(0.05f, 0.05f, 0.08f, 1.0f);
        m_RenderDevice->EndFrame();
    }
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

void Orchestrator::Shutdown()
{
    if (!m_Initialized) return;

    if (m_Client) { m_Client->Disconnect(); m_Client.reset(); }
    if (m_Server) { m_Server->Shutdown();   m_Server.reset(); }

    m_GameWorld.Shutdown();
    m_Level.Unload();
    m_RenderDevice   = nullptr;
    m_Initialized    = false;
    m_LocalClientId  = 0;
    NF::Logger::Log(NF::LogLevel::Info, "Game", "Orchestrator::Shutdown");
}

} // namespace NF::Game
