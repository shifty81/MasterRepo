#include "Runtime/Hazards/HazardSystem.h"
#include <algorithm>
#include <cmath>

namespace Runtime {

void HazardSystem::Init() {
    m_zones.clear();
    m_entities.clear();
    m_inside.clear();
    m_nextId = 1;
}

// ── Hazard management ─────────────────────────────────────────────────────────

uint32_t HazardSystem::AddHazard(const HazardZone& zone) {
    HazardZone z = zone;
    z.id = m_nextId++;
    uint32_t id = z.id;
    m_zones[id] = z;
    return id;
}

void HazardSystem::RemoveHazard(uint32_t hazardId) { m_zones.erase(hazardId); }

void HazardSystem::SetHazardActive(uint32_t hazardId, bool active) {
    auto it = m_zones.find(hazardId);
    if (it != m_zones.end()) it->second.active = active;
}

HazardZone* HazardSystem::GetHazard(uint32_t hazardId) {
    auto it = m_zones.find(hazardId);
    return it != m_zones.end() ? &it->second : nullptr;
}

std::vector<uint32_t> HazardSystem::AllHazardIds() const {
    std::vector<uint32_t> ids;
    for (const auto& [id, _] : m_zones) ids.push_back(id);
    return ids;
}

// ── Entity registration ───────────────────────────────────────────────────────

void HazardSystem::RegisterEntity(uint32_t entityId,
                                   const float* x, const float* y, const float* z) {
    EntityPos ep{entityId, x, y, z};
    m_entities.push_back(ep);
}

void HazardSystem::UnregisterEntity(uint32_t entityId) {
    m_entities.erase(std::remove_if(m_entities.begin(), m_entities.end(),
        [entityId](const EntityPos& e){ return e.entityId == entityId; }),
        m_entities.end());
    // Remove tracking entries
    for (auto it = m_inside.begin(); it != m_inside.end(); ) {
        if ((uint32_t)(it->first >> 32) == entityId)
            it = m_inside.erase(it);
        else
            ++it;
    }
}

// ── Tick ─────────────────────────────────────────────────────────────────────

void HazardSystem::Tick(float dt) {
    for (const auto& ep : m_entities) {
        if (!ep.x || !ep.y || !ep.z) continue;
        float ex = *ep.x, ey = *ep.y, ez = *ep.z;
        for (const auto& [hid, zone] : m_zones) {
            if (!zone.active) continue;
            bool inside = InsideZone(zone, ex, ey, ez);
            uint64_t key = ((uint64_t)ep.entityId << 32) | hid;
            bool wasInside = m_inside.count(key) && m_inside[key];

            if (inside && !wasInside) {
                m_inside[key] = true;
                if (m_onEntered) m_onEntered(ep.entityId, hid);
            } else if (!inside && wasInside) {
                m_inside[key] = false;
                if (m_onExited) m_onExited(ep.entityId, hid);
            }

            if (inside && m_onEffect) {
                HazardEffect eff = ComputeEffect(zone, ep.entityId, dt);
                m_onEffect(eff);
            }
        }
    }
}

// ── Queries ───────────────────────────────────────────────────────────────────

bool HazardSystem::IsInHazard(uint32_t entityId, uint32_t hazardId) const {
    uint64_t key = ((uint64_t)entityId << 32) | hazardId;
    auto it = m_inside.find(key);
    return it != m_inside.end() && it->second;
}

std::vector<uint32_t> HazardSystem::GetHazardsAt(float x, float y, float z) const {
    std::vector<uint32_t> result;
    for (const auto& [id, zone] : m_zones)
        if (zone.active && InsideZone(zone, x, y, z))
            result.push_back(id);
    return result;
}

std::vector<uint32_t> HazardSystem::GetActiveHazardsForEntity(uint32_t entityId) const {
    std::vector<uint32_t> result;
    for (const auto& [key, inside] : m_inside) {
        if (inside && (uint32_t)(key >> 32) == entityId)
            result.push_back((uint32_t)(key & 0xFFFFFFFF));
    }
    return result;
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void HazardSystem::SetEffectCallback(HazardEffectFn fn)   { m_onEffect  = std::move(fn); }
void HazardSystem::SetEnteredCallback(EntityEnteredFn fn) { m_onEntered = std::move(fn); }
void HazardSystem::SetExitedCallback(EntityExitedFn fn)   { m_onExited  = std::move(fn); }

size_t HazardSystem::HazardCount()           const { return m_zones.size(); }
size_t HazardSystem::RegisteredEntityCount() const { return m_entities.size(); }

// ── Private helpers ───────────────────────────────────────────────────────────

bool HazardSystem::InsideZone(const HazardZone& zone,
                                float x, float y, float pz) const {
    float dx = x - zone.cx;
    float dy = y - zone.cy;
    float dz = pz - zone.cz;
    return (dx*dx + dy*dy + dz*dz) <= (zone.radius * zone.radius);
}

HazardEffect HazardSystem::ComputeEffect(const HazardZone& zone,
                                          uint32_t entityId, float dt) const {
    HazardEffect eff;
    eff.entityId = entityId;
    eff.hazardId = zone.id;
    eff.type     = zone.type;

    switch (zone.type) {
    case HazardType::Radiation:
        eff.damagePerSec      = 5.0f  * zone.intensity;
        break;
    case HazardType::AsteroidField:
        // Random collision — approx 2% chance per second
        eff.damagePerSec      = 20.0f * zone.intensity * dt * 0.02f / dt;
        break;
    case HazardType::GravityWell:
        eff.gravAccelX        = zone.intensity * 9.8f; // simplified: always +X
        break;
    case HazardType::Nebula:
        eff.shieldDrainPerSec = 2.0f  * zone.intensity;
        eff.sensorsJammed     = true;
        break;
    case HazardType::IonStorm:
        eff.powerDrained      = true;
        eff.shieldDrainPerSec = 8.0f  * zone.intensity;
        break;
    default: break;
    }
    return eff;
}

} // namespace Runtime
