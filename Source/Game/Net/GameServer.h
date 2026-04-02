#pragma once
#include "Networking/Channel/NetChannel.h"
#include "Networking/Transport/Socket.h"
#include "Game/Net/NetReplicator.h"
#include "Game/World/GameWorld.h"
#include "Game/Movement/PlayerMovement.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <string>

namespace NF::Game {

/// @brief Information about a single connected client.
struct ConnectedClient {
    uint32_t       ClientId{0};
    std::string    PlayerName;
    NF::Socket     ClientSocket;     ///< Socket for this client (moved-in for remote).
    NF::NetChannel Channel;          ///< Framed message channel.
    PlayerMovement Movement;         ///< Server-side authoritative movement.
    NetPlayerState LastState;        ///< Last broadcast state.
    NetClientInput LastInput;        ///< Last received input.
    bool           Connected{false};
    bool           IsLocal{false};   ///< True for in-process (local) clients.
    bool           Welcomed{false};  ///< True after ServerWelcome has been sent.
};

/// @brief Headless authoritative game server.
///
/// Accepts client connections, processes input, ticks the authoritative
/// world simulation, and broadcasts state snapshots.
///
/// This is a logical server that can run in-process (for single-player with
/// server authority) or as a dedicated headless process.
class GameServer {
public:
    GameServer() = default;
    ~GameServer() = default;

    GameServer(const GameServer&)            = delete;
    GameServer& operator=(const GameServer&) = delete;

    // ---- Lifecycle ----------------------------------------------------------

    /// @brief Initialise the server with a game world.
    ///
    /// @param world  The authoritative game world.
    /// @param port   TCP port to listen on (0 = in-process only, no listener).
    /// @return True on success.
    bool Init(GameWorld* world, uint16_t port = 0);

    /// @brief Process one server tick.
    void Tick(float dt);

    /// @brief Shut down the server and disconnect all clients.
    void Shutdown();

    // ---- Client management --------------------------------------------------

    /// @brief Register a local (in-process) client.
    uint32_t AddLocalClient(const std::string& playerName);

    /// @brief Apply input for a local client directly (no network).
    void SubmitLocalInput(uint32_t clientId, const NetClientInput& input);

    /// @brief Get the current state of a specific client.
    [[nodiscard]] const NetPlayerState* GetClientState(uint32_t clientId) const;

    /// @brief Get the last built snapshot.
    [[nodiscard]] const NetWorldSnapshot& GetLastSnapshot() const noexcept { return m_LastSnapshot; }

    // ---- Accessors ----------------------------------------------------------

    [[nodiscard]] size_t   ClientCount() const noexcept { return m_Clients.size(); }
    [[nodiscard]] uint32_t GetTick()     const noexcept { return m_Tick; }
    [[nodiscard]] bool     IsRunning()   const noexcept { return m_Running; }
    [[nodiscard]] uint16_t GetPort()     const noexcept { return m_Port; }

private:
    GameWorld*    m_World{nullptr};
    uint16_t      m_Port{0};
    uint32_t      m_Tick{0};
    uint32_t      m_NextClientId{1};
    bool          m_Running{false};

    NF::Socket    m_ListenSocket;   ///< Server listener (port > 0).

    std::unordered_map<uint32_t, std::unique_ptr<ConnectedClient>> m_Clients;
    NetReplicator m_Replicator;
    NetWorldSnapshot m_LastSnapshot;

    // ---- Internal helpers ---------------------------------------------------

    [[nodiscard]] std::vector<NetPlayerState> GatherPlayerStates() const;

    /// @brief Accept pending TCP connections and register them as clients.
    void AcceptNewConnections();

    /// @brief Receive and decode messages from all remote clients.
    void ReceiveRemoteInput();

    /// @brief Broadcast the current snapshot to all remote clients.
    void BroadcastSnapshot();
};

} // namespace NF::Game
