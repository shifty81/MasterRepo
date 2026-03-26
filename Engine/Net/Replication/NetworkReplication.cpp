#include "Engine/Net/Replication/NetworkReplication.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

namespace Engine {

// ── Per-component registration entry ─────────────────────────────────────────

struct ComponentEntry {
    std::string                                            name;
    NetworkReplication::SerializeFn                        serialize;
    NetworkReplication::DeserializeFn                      deserialize;
    ReplicationPriority                                    priority{ReplicationPriority::Normal};
};

// ── Dirty tracking per entity ─────────────────────────────────────────────────

struct EntityDirtySet {
    std::vector<std::string> dirtyComponents;
};

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct NetworkReplication::Impl {
    std::unordered_map<std::string, ComponentEntry>  components;
    std::unordered_map<uint32_t, EntityDirtySet>     dirty;
    NetworkAuthority                                 authority{NetworkAuthority::Server};
    float                                            interestRange{1000.f};
    uint64_t                                         sequenceNo{0};
    uint64_t                                         bytesSent{0};
    uint64_t                                         bytesRecv{0};

    std::function<void(const ReplicationPacket&)>    onBuilt;
    std::function<void(const ReplicationPacket&)>    onApplied;
};

NetworkReplication::NetworkReplication() : m_impl(new Impl()) {}
NetworkReplication::~NetworkReplication() { delete m_impl; }

void NetworkReplication::Init()     {}
void NetworkReplication::Shutdown() { m_impl->dirty.clear(); m_impl->components.clear(); }

void NetworkReplication::RegisterComponent(const std::string& name,
                                           SerializeFn ser,
                                           DeserializeFn deser,
                                           ReplicationPriority priority) {
    ComponentEntry& e = m_impl->components[name];
    e.name       = name;
    e.serialize  = std::move(ser);
    e.deserialize = std::move(deser);
    e.priority   = priority;
}

void NetworkReplication::UnregisterComponent(const std::string& name) {
    m_impl->components.erase(name);
}

void NetworkReplication::SetAuthority(NetworkAuthority auth) {
    m_impl->authority = auth;
}

NetworkAuthority NetworkReplication::GetAuthority() const {
    return m_impl->authority;
}

void NetworkReplication::SetInterestRange(float r) { m_impl->interestRange = r; }

void NetworkReplication::MarkDirty(uint32_t entityId,
                                   const std::string& component) {
    auto& d = m_impl->dirty[entityId].dirtyComponents;
    if (std::find(d.begin(), d.end(), component) == d.end())
        d.push_back(component);
}

void NetworkReplication::MarkAllDirty(uint32_t entityId) {
    auto& d = m_impl->dirty[entityId].dirtyComponents;
    d.clear();
    for (auto& [name, _] : m_impl->components) d.push_back(name);
}

void NetworkReplication::ClearDirty(uint32_t entityId,
                                    const std::string& component) {
    auto it = m_impl->dirty.find(entityId);
    if (it == m_impl->dirty.end()) return;
    auto& d = it->second.dirtyComponents;
    d.erase(std::remove(d.begin(), d.end(), component), d.end());
}

std::vector<ReplicationPacket> NetworkReplication::BuildDeltaPackets() {
    std::vector<ReplicationPacket> packets;
    for (auto& [entityId, dirtySet] : m_impl->dirty) {
        for (auto& compName : dirtySet.dirtyComponents) {
            auto cit = m_impl->components.find(compName);
            if (cit == m_impl->components.end()) continue;
            ReplicationPacket pkt;
            pkt.entityId       = entityId;
            pkt.componentName  = compName;
            pkt.sequenceNo     = ++m_impl->sequenceNo;
            pkt.isFullSnapshot = false;
            if (cit->second.serialize)
                pkt.payload = cit->second.serialize(entityId);
            m_impl->bytesSent += pkt.payload.size();
            packets.push_back(pkt);
            if (m_impl->onBuilt) m_impl->onBuilt(pkt);
        }
    }
    m_impl->dirty.clear();
    return packets;
}

std::vector<ReplicationPacket>
NetworkReplication::BuildFullSnapshot(uint32_t /*clientId*/) {
    std::vector<ReplicationPacket> packets;
    // For each registered component, serialize all entities that have dirty state
    for (auto& [entityId, _] : m_impl->dirty) {
        for (auto& [compName, entry] : m_impl->components) {
            ReplicationPacket pkt;
            pkt.entityId       = entityId;
            pkt.componentName  = compName;
            pkt.sequenceNo     = ++m_impl->sequenceNo;
            pkt.isFullSnapshot = true;
            if (entry.serialize) pkt.payload = entry.serialize(entityId);
            m_impl->bytesSent += pkt.payload.size();
            packets.push_back(pkt);
        }
    }
    return packets;
}

void NetworkReplication::ApplyIncomingPacket(const uint8_t* data, uint32_t size) {
    // Minimal: treat raw bytes as a single-component delta packet header
    // Real implementation would deserialise a framed multi-packet stream.
    (void)data; (void)size;
    m_impl->bytesRecv += size;
}

void NetworkReplication::ApplyPacket(const ReplicationPacket& pkt) {
    auto it = m_impl->components.find(pkt.componentName);
    if (it == m_impl->components.end()) return;
    if (it->second.deserialize && !pkt.payload.empty())
        it->second.deserialize(pkt.entityId, pkt.payload.data(),
                               static_cast<uint32_t>(pkt.payload.size()));
    m_impl->bytesRecv += pkt.payload.size();
    if (m_impl->onApplied) m_impl->onApplied(pkt);
}

uint32_t NetworkReplication::DirtyComponentCount() const {
    uint32_t n = 0;
    for (auto& [id, d] : m_impl->dirty) n += (uint32_t)d.dirtyComponents.size();
    return n;
}

uint64_t NetworkReplication::BytesSentThisFrame() const { return m_impl->bytesSent; }
uint64_t NetworkReplication::BytesRecvThisFrame() const { return m_impl->bytesRecv; }

void NetworkReplication::ResetFrameStats() {
    m_impl->bytesSent = 0;
    m_impl->bytesRecv = 0;
}

void NetworkReplication::OnPacketBuilt(std::function<void(const ReplicationPacket&)> cb) {
    m_impl->onBuilt = std::move(cb);
}

void NetworkReplication::OnPacketApplied(std::function<void(const ReplicationPacket&)> cb) {
    m_impl->onApplied = std::move(cb);
}

} // namespace Engine
