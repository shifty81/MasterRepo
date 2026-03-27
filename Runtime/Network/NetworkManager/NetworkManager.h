#pragma once
/**
 * @file NetworkManager.h
 * @brief Client/server session management: connect, disconnect, reliable and unreliable messaging.
 *
 * Features:
 *   - Roles: Server (listen/accept), Client (connect), or None
 *   - Connection lifecycle: Connect, Disconnect, OnPeerConnected/Disconnected callbacks
 *   - Reliable ordered channel (like TCP) + unreliable unordered channel (like UDP)
 *   - Message framing: uint16_t message-id + payload (std::vector<uint8_t>)
 *   - Message handler registry: RegisterHandler<MsgId>(callback) per message type
 *   - Broadcast to all peers or unicast to specific peer ID
 *   - Simulated latency/loss for testing (SetSimulation)
 *   - Bandwidth stats: bytes sent/received per second
 *   - Tick-based processing: Tick() drains receive queue and fires handlers
 *   - Transport-agnostic stub (real transport pluggable via SetTransport)
 *
 * Typical usage:
 * @code
 *   NetworkManager nm;
 *   nm.Init(NetworkRole::Server, "0.0.0.0", 7777);
 *   nm.RegisterHandler(MSG_PLAYER_MOVE, [](uint32_t peerId, const uint8_t* data, uint32_t len){
 *       ApplyMove(peerId, data, len);
 *   });
 *   nm.Tick(dt);
 *   // send:
 *   nm.Send(peerId, MSG_SPAWN, payload.data(), (uint32_t)payload.size(), Reliability::Reliable);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class NetworkRole      : uint8_t { None=0, Server, Client };
enum class Reliability      : uint8_t { Reliable=0, Unreliable };
enum class ConnectionState  : uint8_t { Disconnected=0, Connecting, Connected, Disconnecting };

struct PeerInfo {
    uint32_t    peerId{0};
    std::string address;
    uint16_t    port{0};
    ConnectionState state{ConnectionState::Disconnected};
};

struct NetworkStats {
    uint64_t bytesSent{0};
    uint64_t bytesReceived{0};
    uint32_t packetsSent{0};
    uint32_t packetsReceived{0};
    uint32_t packetsLost{0};
    float    rttMs{0.f};
    float    sendKBps{0.f};
    float    recvKBps{0.f};
};

struct SimulationParams {
    float latencyMs{0.f};
    float jitterMs{0.f};
    float packetLoss{0.f};  ///< 0-1 probability
};

using MsgHandler = std::function<void(uint32_t peerId,
                                       const uint8_t* data, uint32_t len)>;

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    void Init    (NetworkRole role, const std::string& address, uint16_t port);
    void Shutdown();
    void Tick    (float dt);

    // Connection
    bool Connect   (const std::string& host, uint16_t port); ///< Client: connect to server
    void Disconnect(uint32_t peerId=0);                       ///< 0 = self / all
    void Listen    ();                                        ///< Server: begin accepting

    // Send
    void Send      (uint32_t peerId, uint16_t msgId,
                    const uint8_t* data, uint32_t len,
                    Reliability rel=Reliability::Reliable);
    void Broadcast (uint16_t msgId,
                    const uint8_t* data, uint32_t len,
                    Reliability rel=Reliability::Reliable);

    // Handler registry
    void RegisterHandler  (uint16_t msgId, MsgHandler handler);
    void UnregisterHandler(uint16_t msgId);

    // Peer queries
    std::vector<PeerInfo> GetPeers()           const;
    const PeerInfo*       GetPeer(uint32_t id) const;
    uint32_t              LocalPeerId()        const;
    ConnectionState       State()              const;
    NetworkRole           Role()               const;

    // Stats
    NetworkStats GetStats(uint32_t peerId=0) const;
    void         ResetStats();

    // Callbacks
    void SetOnPeerConnected   (std::function<void(uint32_t peerId)> cb);
    void SetOnPeerDisconnected(std::function<void(uint32_t peerId)> cb);
    void SetOnConnected       (std::function<void()> cb);
    void SetOnDisconnected    (std::function<void()> cb);

    // Simulation (testing)
    void SetSimulation(const SimulationParams& params);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
