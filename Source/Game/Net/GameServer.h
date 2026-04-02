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
    NF::Socket     ClientSocket;     ///< Socket for this client.
    NF::NetChannel Channel;          ///< Framed message channel.
    PlayerMovement Movement;         ///< Server-side authoritative movement.
    NetPlayerState LastState;        ///< Last broadcast state.
    NetClientInput LastInput;        ///< Last received input.
    bool           Connected{false};
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
    /// The server takes a non-owning reference to the GameWorld; the caller
    /// must ensure the world outlives the server.
    /// @param world  The authoritative game world.
    /// @param port   TCP port to listen on (0 = in-process only).
    /// @return True on success.
    bool Init(GameWorld* world, uint16_t port = 0);

    /// @brief Process one server tick.
    ///
    /// 1. Accept new connections.
    /// 2. Receive and decode client input.
    /// 3. Apply input to each player's authoritative movement.
    /// 4. Tick the world.
    /// 5. Build and broadcast snapshot.
    /// @param dt  Delta time in seconds.
    void Tick(float dt);

    /// @brief Shut down the server and disconnect all clients.
    void Shutdown();

    // ---- Client management --------------------------------------------------

    /// @brief Register a local (in-process) client.
    ///
    /// For single-player with server authority or split-screen.
    /// @param playerName Display name for the client.
    /// @return The assigned client id.
    uint32_t AddLocalClient(const std::string& playerName);

    /// @brief Apply input for a local client directly (no network).
    void SubmitLocalInput(uint32_t clientId, const NetClientInput& input);

    /// @brief Get the current state of a specific client.
    [[nodiscard]] const NetPlayerState* GetClientState(uint32_t clientId) const;

    /// @brief Get the last built snapshot.
    [[nodiscard]] const NetWorldSnapshot& GetLastSnapshot() const noexcept { return m_LastSnapshot; }

    // ---- Accessors ----------------------------------------------------------

    /// @brief Number of connected clients (local + remote).
    [[nodiscard]] size_t ClientCount() const noexcept { return m_Clients.size(); }

    /// @brief Current server tick number.
    [[nodiscard]] uint32_t GetTick() const noexcept { return m_Tick; }

    /// @brief Returns true after Init() and before Shutdown().
    [[nodiscard]] bool IsRunning() const noexcept { return m_Running; }

private:
    GameWorld*    m_World{nullptr};
    uint16_t      m_Port{0};
    uint32_t      m_Tick{0};
    uint32_t      m_NextClientId{1};
    bool          m_Running{false};

    std::unordered_map<uint32_t, std::unique_ptr<ConnectedClient>> m_Clients;
    NetReplicator m_Replicator;
    NetWorldSnapshot m_LastSnapshot;

    /// @brief Gather all player states for snapshot building.
    [[nodiscard]] std::vector<NetPlayerState> GatherPlayerStates() const;
};

} // namespace NF::Game
