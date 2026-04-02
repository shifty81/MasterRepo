#include "Game/App/Orchestrator.h"
#include "Game/World/DevWorldConfig.h"
#include "Game/Voxel/ChunkCoord.h"
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

        // Phase 8: initialise chunk streamer.
        m_Streamer = std::make_unique<ChunkStreamer>();
        ChunkStreamConfig streamCfg;
        streamCfg.LoadRadius   = 4;
        streamCfg.UnloadRadius = 6;
        streamCfg.MaxLoadedChunks = 512;
        streamCfg.MaxLoadsPerTick = 4;
        streamCfg.MaxUnloadsPerTick = 4;
        streamCfg.SaveOnUnload = true;
        m_Streamer->Init(&m_GameWorld.GetChunkMap(), nullptr,
                         streamCfg, m_GameWorld.GetConfig().Seed());
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

    // ---- Audio (Phase 9) ----

    m_AudioDevice.Init(NF::AudioBackend::Null);
    m_AudioMixer.SetSoundBank(&m_SoundBank);
    m_SpatialAudio.SetMixer(&m_AudioMixer);

    // Generate placeholder sounds.
    m_SoundBank.GenerateTone("sfx_mine",    800.f, 0.15f, 0.4f);
    m_SoundBank.GenerateTone("sfx_place",   400.f, 0.2f,  0.3f);
    m_SoundBank.GenerateNoise("sfx_ambient", 2.0f, 0.05f);

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

            // Phase 8: tick chunk streamer with player position.
            if (m_Streamer)
            {
                const auto& pos = m_PlayerMovement.GetPosition();
                const ChunkCoord viewerChunk = WorldToChunk(
                    static_cast<int32_t>(pos.X),
                    static_cast<int32_t>(pos.Y),
                    static_cast<int32_t>(pos.Z));
                m_Streamer->Tick(viewerChunk);
            }

            // Phase 9: update spatial audio listener from player position.
            m_SpatialAudio.SetListenerPosition(m_PlayerMovement.GetPosition());
        }
        else
        {
            // Dedicated server: stream around origin.
            if (m_Streamer)
                m_Streamer->Tick({0, 0, 0});
        }
        break;
    }
    case NetMode::Client:
    {
        if (m_Client) m_Client->Update(dt);
        break;
    }
    }

    // Phase 9: advance audio mixer.
    m_AudioMixer.Update(dt);

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
    if (m_Streamer) { m_Streamer->Shutdown(); m_Streamer.reset(); }

    // Phase 9: shutdown audio.
    m_AudioMixer.StopAll();
    m_SoundBank.Clear();
    m_AudioDevice.Shutdown();

    m_GameWorld.Shutdown();
    m_Level.Unload();
    m_RenderDevice   = nullptr;
    m_Initialized    = false;
    m_LocalClientId  = 0;
    NF::Logger::Log(NF::LogLevel::Info, "Game", "Orchestrator::Shutdown");
}

} // namespace NF::Game
