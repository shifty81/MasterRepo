#include "Engine/Net/P2P/P2PSession/P2PSession.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

// Stub P2P session (no real sockets; simulates peer state management)
namespace Engine {

struct PeerEntry {
    uint32_t  id;
    PeerState state{PeerState::Disconnected};
    float     rttMs{0};
    std::string host;
    uint16_t  port{0};
};

struct P2PSession::Impl {
    bool     hosting{false};
    uint16_t localPort{0};
    uint32_t maxPeers{16};
    uint32_t nextPeerId{1};
    std::vector<PeerEntry> peers;

    std::function<void(uint32_t)> onConnected;
    std::function<void(uint32_t)> onDisconnected;
    std::function<void(uint32_t, const uint8_t*, uint32_t)> onReceive;
    std::function<void(const std::string&)> onError;

    PeerEntry* Find(uint32_t id){
        for(auto& p:peers) if(p.id==id) return &p; return nullptr;
    }
};

P2PSession::P2PSession(): m_impl(new Impl){}
P2PSession::~P2PSession(){ Shutdown(); delete m_impl; }
void P2PSession::Init(){}
void P2PSession::Shutdown(){ m_impl->peers.clear(); m_impl->hosting=false; }
void P2PSession::Reset(){ Shutdown(); m_impl->nextPeerId=1; m_impl->localPort=0; }

bool P2PSession::StartHost(uint16_t port){
    m_impl->hosting=true;
    m_impl->localPort=port;
    return true;
}
bool P2PSession::Connect(const std::string& host, uint16_t port){
    if(m_impl->peers.size()>=m_impl->maxPeers) return false;
    PeerEntry p; p.id=m_impl->nextPeerId++; p.host=host; p.port=port;
    p.state=PeerState::Connected; p.rttMs=20.f;
    m_impl->peers.push_back(p);
    if(m_impl->onConnected) m_impl->onConnected(p.id);
    return true;
}
void P2PSession::Disconnect(uint32_t peerId){
    auto* p=m_impl->Find(peerId); if(!p) return;
    p->state=PeerState::Disconnected;
    if(m_impl->onDisconnected) m_impl->onDisconnected(peerId);
    m_impl->peers.erase(std::remove_if(m_impl->peers.begin(),m_impl->peers.end(),
        [peerId](const PeerEntry& pe){return pe.id==peerId;}),m_impl->peers.end());
}
void P2PSession::DisconnectAll(){
    std::vector<uint32_t> ids;
    for(auto& p:m_impl->peers) ids.push_back(p.id);
    for(auto id:ids) Disconnect(id);
}

void P2PSession::SendReliable  (uint32_t, const uint8_t*, uint32_t){}
void P2PSession::SendUnreliable(uint32_t, const uint8_t*, uint32_t){}
void P2PSession::Broadcast     (const uint8_t* data, uint32_t size, bool){
    for(auto& p:m_impl->peers)
        if(m_impl->onReceive) m_impl->onReceive(p.id, data, size);
}

void P2PSession::Poll(float dt){
    // Simulate occasional RTT jitter
    for(auto& p:m_impl->peers)
        p.rttMs=20.f+5.f*std::sin(p.id*0.7f+dt);
}

uint32_t  P2PSession::GetPeerCount  () const { return (uint32_t)m_impl->peers.size(); }
uint32_t  P2PSession::GetPeerId     (uint32_t i) const { return i<m_impl->peers.size()?m_impl->peers[i].id:0; }
float     P2PSession::GetPeerRTT    (uint32_t id) const { auto* p=m_impl->Find(id); return p?p->rttMs:0; }
PeerState P2PSession::GetPeerState  (uint32_t id) const { auto* p=m_impl->Find(id); return p?p->state:PeerState::Disconnected; }
uint16_t  P2PSession::GetLocalPort  () const { return m_impl->localPort; }
bool      P2PSession::IsHost        () const { return m_impl->hosting; }
void      P2PSession::SetMaxPeers   (uint32_t n){ m_impl->maxPeers=n; }

void P2PSession::SetOnConnected   (std::function<void(uint32_t)> cb){ m_impl->onConnected=cb; }
void P2PSession::SetOnDisconnected(std::function<void(uint32_t)> cb){ m_impl->onDisconnected=cb; }
void P2PSession::SetOnReceive(std::function<void(uint32_t,const uint8_t*,uint32_t)> cb){ m_impl->onReceive=cb; }
void P2PSession::SetOnError(std::function<void(const std::string&)> cb){ m_impl->onError=cb; }

} // namespace Engine
