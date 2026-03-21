#include "Engine/Network/NetworkManager.h"
#include <algorithm>

// Stub implementation — no external networking library required.
// Replace Impl internals with ENet / LiteNetLib / asio when integrating a
// real networking stack; the public API remains unchanged.

namespace Engine {

struct NetworkManager::Impl {
    bool hosting{false};
    bool connected{false};
    uint16_t port{0};
    std::string remoteHost;
    std::vector<PeerID>           peers;
    std::vector<NetEventCallback> callbacks;
    uint32_t nextPeer{1};
};

NetworkManager::NetworkManager() : m_impl(new Impl()) {}
NetworkManager::~NetworkManager() { Shutdown(); delete m_impl; }

bool NetworkManager::Host(uint16_t port, int /*maxPeers*/) {
    if (m_impl->hosting || m_impl->connected) return false;
    m_impl->hosting = true;
    m_impl->port    = port;
    return true;
}

bool NetworkManager::Connect(const std::string& host, uint16_t port) {
    if (m_impl->hosting || m_impl->connected) return false;
    m_impl->remoteHost = host;
    m_impl->port       = port;
    m_impl->connected  = true;
    PeerID id = m_impl->nextPeer++;
    m_impl->peers.push_back(id);
    NetEvent ev;
    ev.type   = NetEventType::Connect;
    ev.peer   = id;
    for (auto& cb : m_impl->callbacks) cb(ev);
    return true;
}

void NetworkManager::Shutdown() {
    if (!m_impl->hosting && !m_impl->connected) return;
    for (PeerID p : m_impl->peers) {
        NetEvent ev;
        ev.type = NetEventType::Disconnect;
        ev.peer = p;
        for (auto& cb : m_impl->callbacks) cb(ev);
    }
    m_impl->peers.clear();
    m_impl->hosting   = false;
    m_impl->connected = false;
}

void NetworkManager::Poll(uint32_t /*timeoutMs*/) {
    // Stub: real implementation calls enet_host_service() or equivalent.
}

bool NetworkManager::Send(PeerID peer, const Packet& pkt) {
    auto it = std::find(m_impl->peers.begin(), m_impl->peers.end(), peer);
    return it != m_impl->peers.end() && !pkt.data.empty();
}

void NetworkManager::Broadcast(const Packet& pkt) {
    for (PeerID p : m_impl->peers) Send(p, pkt);
}

void NetworkManager::OnEvent(NetEventCallback cb) {
    m_impl->callbacks.push_back(std::move(cb));
}

bool   NetworkManager::IsHosting()    const { return m_impl->hosting; }
bool   NetworkManager::IsConnected()  const { return m_impl->connected; }
std::vector<PeerID> NetworkManager::GetPeers() const { return m_impl->peers; }
size_t NetworkManager::PeerCount()    const { return m_impl->peers.size(); }
std::string NetworkManager::LocalAddress() const {
    return m_impl->hosting
        ? "0.0.0.0:" + std::to_string(m_impl->port)
        : "";
}

} // namespace Engine
