#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace Engine {

/// Peer identifier
using PeerID = uint32_t;

/// Raw packet payload
struct Packet {
    std::vector<uint8_t> data;
    PeerID               sender{0};
    uint8_t              channel{0};
    bool                 reliable{true};
};

/// Network event types
enum class NetEventType { Connect, Disconnect, Receive, Timeout };

struct NetEvent {
    NetEventType type{NetEventType::Connect};
    PeerID       peer{0};
    Packet       packet;
};

/// Callbacks
using NetEventCallback = std::function<void(const NetEvent&)>;

/// NetworkManager — lightweight session manager (ENet-style, header-only API)
///
/// Supports server (listen), client (connect), and peer-to-peer modes.
/// All I/O is non-blocking; call Poll() each tick.
class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    // ── lifecycle ──────────────────────────────────────────────
    /// Start a server on the given port, accepting up to maxPeers clients.
    bool Host(uint16_t port, int maxPeers = 16);
    /// Connect to a remote server.
    bool Connect(const std::string& host, uint16_t port);
    /// Stop hosting / disconnect all peers and free resources.
    void Shutdown();

    // ── per-tick ──────────────────────────────────────────────
    /// Service the network; fires registered callbacks for each event.
    /// Call once per game tick. timeoutMs=0 returns immediately.
    void Poll(uint32_t timeoutMs = 0);

    // ── messaging ─────────────────────────────────────────────
    /// Send a packet to a specific peer (server→client or client→server).
    bool Send(PeerID peer, const Packet& pkt);
    /// Broadcast to all connected peers.
    void Broadcast(const Packet& pkt);

    // ── registration ──────────────────────────────────────────
    void OnEvent(NetEventCallback cb);

    // ── query ─────────────────────────────────────────────────
    bool              IsHosting()   const;
    bool              IsConnected() const;
    std::vector<PeerID> GetPeers()  const;
    size_t            PeerCount()   const;
    std::string       LocalAddress() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
