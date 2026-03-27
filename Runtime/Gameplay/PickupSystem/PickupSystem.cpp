#include "Runtime/Gameplay/PickupSystem/PickupSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct PickupSystem::Impl {
    std::unordered_map<std::string, PickupTypeDesc> types;

    std::unordered_map<uint32_t, PickupInstance> pickups;
    uint32_t nextPickupId{1};

    struct CollectorRecord {
        uint32_t    id{0};
        float       position[3]{};
        std::string teamTag;
        std::unordered_map<std::string,uint32_t> collected; ///< typeId → count
    };
    std::unordered_map<uint32_t, CollectorRecord> collectors;

    CollectCb  onCollect;
    SpawnCb    onSpawn;
    DespawnCb  onDespawn;

    float Dist(const float a[3], const float b[3]) const {
        float dx=a[0]-b[0], dy=a[1]-b[1], dz=a[2]-b[2];
        return std::sqrt(dx*dx+dy*dy+dz*dz);
    }
};

PickupSystem::PickupSystem()  : m_impl(new Impl) {}
PickupSystem::~PickupSystem() { Shutdown(); delete m_impl; }

void PickupSystem::Init()     {}
void PickupSystem::Shutdown() { m_impl->pickups.clear(); }

void PickupSystem::RegisterType(const PickupTypeDesc& desc)
{
    m_impl->types[desc.typeId] = desc;
}

void PickupSystem::UnregisterType(const std::string& typeId)
{
    m_impl->types.erase(typeId);
}

bool PickupSystem::HasType(const std::string& typeId) const
{
    return m_impl->types.count(typeId) > 0;
}

uint32_t PickupSystem::Spawn(const std::string& typeId, const float position[3])
{
    uint32_t id = m_impl->nextPickupId++;
    PickupInstance inst;
    inst.pickupId = id;
    inst.typeId   = typeId;
    inst.position[0]=position[0]; inst.position[1]=position[1]; inst.position[2]=position[2];
    inst.active = true;
    m_impl->pickups[id] = inst;
    if (m_impl->onSpawn) m_impl->onSpawn(id, typeId);
    return id;
}

void PickupSystem::Despawn(uint32_t pickupId)
{
    auto it = m_impl->pickups.find(pickupId);
    if (it == m_impl->pickups.end()) return;
    it->second.active = false;
    if (m_impl->onDespawn) m_impl->onDespawn(pickupId);
    m_impl->pickups.erase(it);
}

void PickupSystem::DespawnAll(const std::string& typeId)
{
    std::vector<uint32_t> toRemove;
    for (auto& [id,pk] : m_impl->pickups)
        if (typeId.empty() || pk.typeId == typeId) toRemove.push_back(id);
    for (auto id : toRemove) Despawn(id);
}

bool PickupSystem::IsActive(uint32_t pickupId) const
{
    auto it = m_impl->pickups.find(pickupId);
    return it != m_impl->pickups.end() && it->second.active;
}

const PickupInstance* PickupSystem::GetInstance(uint32_t pickupId) const
{
    auto it = m_impl->pickups.find(pickupId);
    return it != m_impl->pickups.end() ? &it->second : nullptr;
}

void PickupSystem::SetCollectorPosition(uint32_t collectorId, const float position[3])
{
    auto& c = m_impl->collectors[collectorId];
    c.id = collectorId;
    c.position[0]=position[0]; c.position[1]=position[1]; c.position[2]=position[2];
}

void PickupSystem::SetCollectorTeam(uint32_t collectorId, const std::string& teamTag)
{
    m_impl->collectors[collectorId].teamTag = teamTag;
}

void PickupSystem::RemoveCollector(uint32_t collectorId)
{
    m_impl->collectors.erase(collectorId);
}

void PickupSystem::Tick(float dt)
{
    // Respawn timers
    for (auto& [id, pk] : m_impl->pickups) {
        if (pk.active) continue;
        auto tit = m_impl->types.find(pk.typeId);
        if (tit == m_impl->types.end() || tit->second.respawnDelay <= 0.f) continue;
        pk.respawnTimer -= dt;
        if (pk.respawnTimer <= 0.f) {
            pk.active = true;
            pk.respawnTimer = 0.f;
            if (m_impl->onSpawn) m_impl->onSpawn(id, pk.typeId);
        }
    }

    // Proximity collection
    std::vector<uint32_t> toCollect;
    for (auto& [cid, col] : m_impl->collectors) {
        for (auto& [pid, pk] : m_impl->pickups) {
            if (!pk.active) continue;
            auto tit = m_impl->types.find(pk.typeId);
            if (tit == m_impl->types.end()) continue;
            const auto& type = tit->second;

            // Team check
            if (!type.teamTag.empty() && col.teamTag != type.teamTag) continue;

            float dist = m_impl->Dist(col.position, pk.position);

            // Magnet
            if (type.magnetRadius > 0.f && dist <= type.magnetRadius) {
                float step = type.magnetSpeed * dt;
                for (int i=0;i<3;i++) {
                    float d = col.position[i] - pk.position[i];
                    float absD = std::abs(d);
                    if (absD > 0.01f) pk.position[i] += (d/absD) * std::min(step, absD);
                }
                dist = m_impl->Dist(col.position, pk.position);
            }

            if (dist > type.collectRadius) continue;

            // One-shot check
            if (type.oneShot) {
                if (col.collected.count(pk.typeId) &&
                    col.collected[pk.typeId] > 0) continue;
            }

            // Max carry check
            if (type.maxCarry > 0) {
                if (col.collected[pk.typeId] >= type.maxCarry) continue;
            }

            col.collected[pk.typeId]++;
            toCollect.push_back(pid);

            CollectEvent evt;
            evt.collectorId = cid;
            evt.pickupId    = pid;
            evt.typeId      = pk.typeId;
            evt.category    = type.category;
            evt.pickupValue = type.value;
            evt.position[0]=pk.position[0]; evt.position[1]=pk.position[1]; evt.position[2]=pk.position[2];
            if (m_impl->onCollect) m_impl->onCollect(evt);
        }
    }

    for (auto pid : toCollect) {
        auto it = m_impl->pickups.find(pid);
        if (it == m_impl->pickups.end()) continue;
        const auto& type = m_impl->types[it->second.typeId];
        if (type.respawnDelay > 0.f) {
            it->second.active = false;
            it->second.respawnTimer = type.respawnDelay;
            if (m_impl->onDespawn) m_impl->onDespawn(pid);
        } else {
            Despawn(pid);
        }
    }
}

std::vector<uint32_t> PickupSystem::GetPickupsInRadius(const float centre[3],
                                                         float radius) const
{
    std::vector<uint32_t> out;
    for (auto& [id,pk] : m_impl->pickups) {
        if (!pk.active) continue;
        if (m_impl->Dist(centre, pk.position) <= radius) out.push_back(id);
    }
    return out;
}

std::vector<uint32_t> PickupSystem::GetActivePickups(const std::string& typeId) const
{
    std::vector<uint32_t> out;
    for (auto& [id,pk] : m_impl->pickups) {
        if (!pk.active) continue;
        if (!typeId.empty() && pk.typeId != typeId) continue;
        out.push_back(id);
    }
    return out;
}

void PickupSystem::SetOnCollected(CollectCb cb)  { m_impl->onCollect  = cb; }
void PickupSystem::SetOnSpawned(SpawnCb cb)       { m_impl->onSpawn    = cb; }
void PickupSystem::SetOnDespawned(DespawnCb cb)   { m_impl->onDespawn  = cb; }

} // namespace Runtime
