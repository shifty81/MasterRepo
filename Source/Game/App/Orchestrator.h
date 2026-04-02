#pragma once
#include "Engine/World/Level.h"
#include "Renderer/RHI/RenderDevice.h"
#include "Game/World/GameWorld.h"
#include "Game/Interaction/InteractionLoop.h"
#include "Game/Movement/PlayerMovement.h"
#include "Game/Net/GameServer.h"
#include "Game/Net/GameClient.h"
#include "Game/Voxel/ChunkStreamer.h"
#include "Audio/Core/AudioDevice.h"
#include "Audio/Core/SoundBank.h"
#include "Audio/Mixer/AudioMixer.h"
#include "Audio/Spatial/SpatialAudio.h"
#include <memory>
#include <string>

namespace NF::Game {

/// @brief Network mode for the Orchestrator session.
enum class NetMode : uint8_t {
    Solo          = 0,  ///< Single-player, no networking.
    ListenServer  = 1,  ///< Host plays while serving remote clients.
    Dedicated     = 2,  ///< Headless server only — no local player.
    Client        = 3,  ///< Remote client connecting to a server.
};

/// @brief Connection parameters for networked modes.
struct NetParams {
    NetMode     Mode{NetMode::Solo};
    std::string Host{"127.0.0.1"};   ///< Server address (Client mode).
    uint16_t    Port{7777};           ///< Listen port (server) or connect port (client).
    std::string PlayerName{"Player"};
};

/// @brief Top-level game runtime orchestrator.
///
/// Owns the active @c Level, the game world (voxel + ECS), the Phase 3
/// interaction loop, and the Phase 5 player movement controller.
/// In Phase 7 also owns GameServer / GameClient depending on the
/// active @c NetMode.
///
/// Holds a non-owning pointer to the @c RenderDevice provided at Init()
/// time.  Call Init(), then loop Tick(), then Shutdown().
class Orchestrator {
public:
    Orchestrator() = default;
    ~Orchestrator() = default;

    Orchestrator(const Orchestrator&)            = delete;
    Orchestrator& operator=(const Orchestrator&) = delete;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// @brief Initialise in Solo mode (backward compatible).
    bool Init(RenderDevice* renderDevice);

    /// @brief Initialise with explicit net mode and connection parameters.
    bool Init(RenderDevice* renderDevice, const NetParams& params);

    /// @brief Advance the game simulation by one variable-rate tick.
    void Tick(float dt);

    /// @brief Tear down all runtime subsystems.
    void Shutdown();

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    [[nodiscard]] Level*       GetLevel()       noexcept { return &m_Level; }
    [[nodiscard]] const Level* GetLevel() const noexcept { return &m_Level; }

    [[nodiscard]] GameWorld&       GetGameWorld()       noexcept { return m_GameWorld; }
    [[nodiscard]] const GameWorld& GetGameWorld() const noexcept { return m_GameWorld; }

    [[nodiscard]] InteractionLoop&       GetInteractionLoop()       noexcept { return m_InteractionLoop; }
    [[nodiscard]] const InteractionLoop& GetInteractionLoop() const noexcept { return m_InteractionLoop; }

    [[nodiscard]] PlayerMovement&       GetPlayerMovement()       noexcept { return m_PlayerMovement; }
    [[nodiscard]] const PlayerMovement& GetPlayerMovement() const noexcept { return m_PlayerMovement; }

    [[nodiscard]] bool IsInitialized() const noexcept { return m_Initialized; }
    [[nodiscard]] NetMode GetNetMode() const noexcept { return m_NetMode; }

    /// @brief Returns the chunk streamer (non-null after Init for server modes).
    [[nodiscard]] ChunkStreamer* GetChunkStreamer() noexcept { return m_Streamer.get(); }

    /// @brief Returns the audio device (always available after Init).
    [[nodiscard]] AudioDevice&       GetAudioDevice()       noexcept { return m_AudioDevice; }
    [[nodiscard]] const AudioDevice& GetAudioDevice() const noexcept { return m_AudioDevice; }

    /// @brief Returns the sound bank.
    [[nodiscard]] SoundBank&       GetSoundBank()       noexcept { return m_SoundBank; }
    [[nodiscard]] const SoundBank& GetSoundBank() const noexcept { return m_SoundBank; }

    /// @brief Returns the audio mixer.
    [[nodiscard]] AudioMixer&       GetAudioMixer()       noexcept { return m_AudioMixer; }
    [[nodiscard]] const AudioMixer& GetAudioMixer() const noexcept { return m_AudioMixer; }

    /// @brief Returns the spatial audio manager.
    [[nodiscard]] SpatialAudio&       GetSpatialAudio()       noexcept { return m_SpatialAudio; }
    [[nodiscard]] const SpatialAudio& GetSpatialAudio() const noexcept { return m_SpatialAudio; }

    /// @brief Returns the GameServer (non-null when Solo/ListenServer/Dedicated).
    [[nodiscard]] GameServer* GetServer() noexcept { return m_Server.get(); }
    /// @brief Returns the GameClient (non-null when ListenServer/Client).
    [[nodiscard]] GameClient* GetClient() noexcept { return m_Client.get(); }

    /// @brief Local client id on the server (Solo / ListenServer only).
    [[nodiscard]] uint32_t GetLocalClientId() const noexcept { return m_LocalClientId; }

private:
    Level            m_Level;
    GameWorld        m_GameWorld;
    InteractionLoop  m_InteractionLoop;
    PlayerMovement   m_PlayerMovement;
    RenderDevice*    m_RenderDevice{nullptr};
    bool             m_Initialized{false};

    // Phase 7 networking
    NetMode                      m_NetMode{NetMode::Solo};
    std::unique_ptr<GameServer>  m_Server;
    std::unique_ptr<GameClient>  m_Client;
    uint32_t                     m_LocalClientId{0};
    NetParams                    m_NetParams;

    // Phase 8 chunk streaming
    std::unique_ptr<ChunkStreamer> m_Streamer;

    // Phase 9 audio
    AudioDevice  m_AudioDevice;
    SoundBank    m_SoundBank;
    AudioMixer   m_AudioMixer;
    SpatialAudio m_SpatialAudio;
};

} // namespace NF::Game
