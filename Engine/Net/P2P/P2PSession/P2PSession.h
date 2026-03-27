#pragma once
/**
 * @file P2PSession.h
 * @brief Peer-to-peer networking session: connection management, reliable/unreliable channels.
 *
 * Features:
 *   - StartHost(port) → bool: open a listening socket
 *   - Connect(host, port) → bool: connect to a remote peer
 *   - Disconnect(peerId) / DisconnectAll()
 *   - SendReliable(peerId, data, size): ordered, delivery-guaranteed channel
 *   - SendUnreliable(peerId, data, size): best-effort low-latency channel
 *   - Broadcast(data, size, reliable)
 *   - Poll(dt): process incoming packets, fire callbacks
 *   - GetPeerCount() → uint32_t
 *   - GetPeerId(index) → uint32_t
 *   - GetPeerRTT(peerId) → float ms
 *   - GetPeerState(peerId) → PeerState: {Disconnected, Connecting, Connected}
 *   - SetOnConnected(cb): callback(peerId)
 *   - SetOnDisconnected(cb): callback(peerId)
 *   - SetOnReceive(cb): callback(peerId, data*, size)
 *   - SetOnError(cb): callback(message)
 *   - SetMaxPeers(n): cap simultaneous connections
 *   - GetLocalPort() → uint16_t
 *   - IsHost() → bool
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class PeerState : uint8_t { Disconnected, Connecting, Connected };

class P2PSession {
public:
    P2PSession();
    ~P2PSession();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Session management
    bool StartHost (uint16_t port);
    bool Connect   (const std::string& host, uint16_t port);
    void Disconnect(uint32_t peerId);
    void DisconnectAll();

    // Send
    void SendReliable  (uint32_t peerId, const uint8_t* data, uint32_t size);
    void SendUnreliable(uint32_t peerId, const uint8_t* data, uint32_t size);
    void Broadcast     (const uint8_t* data, uint32_t size, bool reliable);

    // Per-frame
    void Poll(float dt);

    // Query
    uint32_t  GetPeerCount() const;
    uint32_t  GetPeerId   (uint32_t index) const;
    float     GetPeerRTT  (uint32_t peerId) const;
    PeerState GetPeerState(uint32_t peerId) const;
    uint16_t  GetLocalPort() const;
    bool      IsHost      () const;

    // Config
    void SetMaxPeers(uint32_t n);

    // Callbacks
    void SetOnConnected   (std::function<void(uint32_t peerId)> cb);
    void SetOnDisconnected(std::function<void(uint32_t peerId)> cb);
    void SetOnReceive     (std::function<void(uint32_t peerId,
                                              const uint8_t* data, uint32_t size)> cb);
    void SetOnError       (std::function<void(const std::string&)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
