#include "Engine/Net/NetContext.h"

namespace Engine::net {

struct NetContext::Impl {
    NetRole        role{NetRole::None};
    uint16_t       port{7777};
    std::string    remoteHost{"127.0.0.1"};
    int            maxPeers{16};
    bool           running{false};
    NetworkManager mgr;
};

NetContext::NetContext() : m_impl(new Impl()) {}
NetContext::~NetContext() { Stop(); delete m_impl; }

void NetContext::SetRole(NetRole role)         { m_impl->role = role; }
NetRole NetContext::GetRole() const            { return m_impl->role; }
void NetContext::SetPort(uint16_t port)        { m_impl->port = port; }
uint16_t NetContext::GetPort() const           { return m_impl->port; }
void NetContext::SetRemoteHost(const std::string& h) { m_impl->remoteHost = h; }
std::string NetContext::GetRemoteHost() const  { return m_impl->remoteHost; }
void NetContext::SetMaxPeers(int max)          { m_impl->maxPeers = max; }

bool NetContext::Start() {
    if (m_impl->running) return true;
    bool ok = false;
    switch (m_impl->role) {
    case NetRole::None:
        return true; // networking intentionally disabled
    case NetRole::ListenServer:
    case NetRole::DedicatedServer:
        ok = m_impl->mgr.Host(m_impl->port, m_impl->maxPeers);
        break;
    case NetRole::Client:
        ok = m_impl->mgr.Connect(m_impl->remoteHost, m_impl->port);
        break;
    }
    m_impl->running = ok;
    return ok;
}

void NetContext::Stop() {
    if (!m_impl->running) return;
    m_impl->mgr.Shutdown();
    m_impl->running = false;
}

bool NetContext::IsRunning() const { return m_impl->running; }

void NetContext::Poll() {
    if (m_impl->running) m_impl->mgr.Poll();
}

void NetContext::Broadcast(const std::vector<uint8_t>& data, bool reliable) {
    if (!m_impl->running) return;
    Packet pkt;
    pkt.data     = data;
    pkt.reliable = reliable;
    m_impl->mgr.Broadcast(pkt);
}

void NetContext::SendTo(PeerID peer, const std::vector<uint8_t>& data, bool reliable) {
    if (!m_impl->running) return;
    Packet pkt;
    pkt.data     = data;
    pkt.reliable = reliable;
    m_impl->mgr.Send(peer, pkt);
}

void NetContext::OnEvent(NetEventCallback cb) {
    m_impl->mgr.OnEvent(std::move(cb));
}

NetworkManager& NetContext::GetManager()             { return m_impl->mgr; }
const NetworkManager& NetContext::GetManager() const { return m_impl->mgr; }

} // namespace Engine::net
