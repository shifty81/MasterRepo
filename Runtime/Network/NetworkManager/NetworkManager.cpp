#include "Runtime/Network/NetworkManager/NetworkManager.h"
#include <algorithm>
#include <cstring>
#include <queue>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct PendingMsg {
    uint32_t             peerId{0};
    uint16_t             msgId{0};
    std::vector<uint8_t> data;
    Reliability          rel{Reliability::Reliable};
};

struct NetworkManager::Impl {
    NetworkRole     role  {NetworkRole::None};
    std::string     address;
    uint16_t        port  {0};
    ConnectionState state {ConnectionState::Disconnected};
    uint32_t        localPeerId{0};
    uint32_t        nextPeerId {1};

    std::vector<PeerInfo>    peers;
    std::queue<PendingMsg>   inbound;
    std::queue<PendingMsg>   outbound;

    std::unordered_map<uint16_t, MsgHandler> handlers;

    NetworkStats   globalStats;
    SimulationParams simParams;

    std::function<void(uint32_t)> onPeerConnected;
    std::function<void(uint32_t)> onPeerDisconnected;
    std::function<void()>         onConnected;
    std::function<void()>         onDisconnected;

    PeerInfo* FindPeer(uint32_t id) {
        for (auto& p : peers) if (p.peerId==id) return &p;
        return nullptr;
    }
};

NetworkManager::NetworkManager()  : m_impl(new Impl) {}
NetworkManager::~NetworkManager() { Shutdown(); delete m_impl; }

void NetworkManager::Init(NetworkRole role, const std::string& address, uint16_t port)
{
    m_impl->role    = role;
    m_impl->address = address;
    m_impl->port    = port;
    m_impl->localPeerId = m_impl->nextPeerId++;
}

void NetworkManager::Shutdown()
{
    m_impl->peers.clear();
    m_impl->state = ConnectionState::Disconnected;
}

bool NetworkManager::Connect(const std::string& host, uint16_t port)
{
    m_impl->address = host;
    m_impl->port    = port;
    m_impl->state   = ConnectionState::Connected;
    // Simulate server peer
    PeerInfo server;
    server.peerId = m_impl->nextPeerId++;
    server.address= host;
    server.port   = port;
    server.state  = ConnectionState::Connected;
    m_impl->peers.push_back(server);
    if (m_impl->onConnected) m_impl->onConnected();
    if (m_impl->onPeerConnected) m_impl->onPeerConnected(server.peerId);
    return true;
}

void NetworkManager::Listen() { m_impl->state = ConnectionState::Connected; }

void NetworkManager::Disconnect(uint32_t peerId)
{
    if (peerId == 0) {
        for (auto& p : m_impl->peers)
            if (m_impl->onPeerDisconnected) m_impl->onPeerDisconnected(p.peerId);
        m_impl->peers.clear();
        m_impl->state = ConnectionState::Disconnected;
        if (m_impl->onDisconnected) m_impl->onDisconnected();
    } else {
        auto* p = m_impl->FindPeer(peerId);
        if (p) {
            p->state = ConnectionState::Disconnected;
            if (m_impl->onPeerDisconnected) m_impl->onPeerDisconnected(peerId);
            m_impl->peers.erase(std::remove_if(m_impl->peers.begin(), m_impl->peers.end(),
                [&](auto& x){ return x.peerId==peerId; }), m_impl->peers.end());
        }
    }
}

void NetworkManager::Send(uint32_t peerId, uint16_t msgId,
                           const uint8_t* data, uint32_t len, Reliability rel)
{
    PendingMsg msg;
    msg.peerId=peerId; msg.msgId=msgId; msg.rel=rel;
    msg.data.assign(data, data+len);
    m_impl->outbound.push(std::move(msg));
    m_impl->globalStats.packetsSent++;
    m_impl->globalStats.bytesSent += len;
}

void NetworkManager::Broadcast(uint16_t msgId, const uint8_t* data, uint32_t len, Reliability rel)
{
    for (auto& p : m_impl->peers) Send(p.peerId, msgId, data, len, rel);
}

void NetworkManager::RegisterHandler  (uint16_t msgId, MsgHandler h) { m_impl->handlers[msgId]=h; }
void NetworkManager::UnregisterHandler(uint16_t msgId)               { m_impl->handlers.erase(msgId); }

std::vector<PeerInfo> NetworkManager::GetPeers()           const { return m_impl->peers; }
const PeerInfo*       NetworkManager::GetPeer(uint32_t id) const { return m_impl->FindPeer(id); }
uint32_t              NetworkManager::LocalPeerId()        const { return m_impl->localPeerId; }
ConnectionState       NetworkManager::State()              const { return m_impl->state; }
NetworkRole           NetworkManager::Role()               const { return m_impl->role; }

NetworkStats NetworkManager::GetStats(uint32_t /*peerId*/) const { return m_impl->globalStats; }
void         NetworkManager::ResetStats() { m_impl->globalStats = {}; }

void NetworkManager::SetOnPeerConnected   (std::function<void(uint32_t)> cb) { m_impl->onPeerConnected=cb; }
void NetworkManager::SetOnPeerDisconnected(std::function<void(uint32_t)> cb) { m_impl->onPeerDisconnected=cb; }
void NetworkManager::SetOnConnected       (std::function<void()>         cb) { m_impl->onConnected=cb; }
void NetworkManager::SetOnDisconnected    (std::function<void()>         cb) { m_impl->onDisconnected=cb; }
void NetworkManager::SetSimulation        (const SimulationParams& p)        { m_impl->simParams=p; }

void NetworkManager::Tick(float dt)
{
    // Simulate loopback: move outbound to inbound
    while (!m_impl->outbound.empty()) {
        auto msg = m_impl->outbound.front(); m_impl->outbound.pop();
        m_impl->inbound.push(msg);
        m_impl->globalStats.packetsReceived++;
        m_impl->globalStats.bytesReceived += (uint64_t)msg.data.size();
    }
    // Dispatch inbound
    while (!m_impl->inbound.empty()) {
        auto msg = m_impl->inbound.front(); m_impl->inbound.pop();
        auto it  = m_impl->handlers.find(msg.msgId);
        if (it != m_impl->handlers.end() && it->second)
            it->second(msg.peerId, msg.data.data(), (uint32_t)msg.data.size());
    }
    (void)dt;
}

} // namespace Runtime
