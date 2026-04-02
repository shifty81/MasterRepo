#pragma once
#include "Networking/Channel/NetChannel.h"
#include "Networking/Transport/Socket.h"
#include "Game/Net/NetReplicator.h"
#include <cstdint>
#include <string>

namespace NF::Game {

/// @brief Connection state for the game client.
enum class GameClientState : uint8_t {
    Disconnected = 0,  ///< Not connected to any server.
    Connecting   = 1,  ///< Handshake in progress.
    Connected    = 2,  ///< Live session with server.
};

/// @brief Game network client — connects to a GameServer, sends input,
///        receives authoritative state.
///
/// Manages the client-side session lifecycle: connect → send input →
/// receive snapshots → disconnect.  The client does NOT own the world;
/// it applies received snapshots to update the local prediction.
class GameClient {
public:
    GameClient() = default;
    ~GameClient() = default;

    GameClient(const GameClient&)            = delete;
    GameClient& operator=(const GameClient&) = delete;

    // ---- Lifecycle ----------------------------------------------------------

    /// @brief Connect to a server at the given host and port.
    /// @return True if the connection was initiated.
    bool Connect(const std::string& host, uint16_t port,
                 const std::string& playerName);

    /// @brief Gracefully disconnect from the server.
    void Disconnect();

    /// @brief Per-frame network update.
    ///
    /// 1. Flush queued input to the server.
    /// 2. Poll for incoming snapshots.
    /// 3. Apply the latest snapshot.
    /// @param dt Delta time in seconds.
    void Update(float dt);

    // ---- Input submission ---------------------------------------------------

    /// @brief Queue the current frame's input for sending to the server.
    void SubmitInput(const NetClientInput& input);

    // ---- State access -------------------------------------------------------

    [[nodiscard]] GameClientState GetState()   const noexcept { return m_State; }
    [[nodiscard]] bool IsConnected()           const noexcept { return m_State == GameClientState::Connected; }
    [[nodiscard]] uint32_t GetClientId()       const noexcept { return m_ClientId; }

    /// @brief The most recently applied world snapshot.
    [[nodiscard]] const NetWorldSnapshot& GetLastSnapshot() const noexcept { return m_LastSnapshot; }

    /// @brief Get the local player's replicated state from the last snapshot.
    /// @return Pointer to the local player state, or nullptr if not found.
    [[nodiscard]] const NetPlayerState* GetLocalPlayerState() const;

private:
    GameClientState m_State{GameClientState::Disconnected};
    uint32_t        m_ClientId{0};
    std::string     m_PlayerName;

    NF::Socket      m_Socket;
    NF::NetChannel  m_Channel;
    NetReplicator   m_Replicator;
    NetWorldSnapshot m_LastSnapshot;
    uint32_t        m_InputSeq{0};
};

} // namespace NF::Game
