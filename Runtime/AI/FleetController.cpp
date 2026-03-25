#include "Runtime/AI/FleetController.h"
#include <algorithm>
#include <cmath>

namespace Runtime {

void FleetController::Init() {
    m_fleets.clear();
    m_nextFleetId = 1;
}

// ── Fleet management ──────────────────────────────────────────────────────────

uint32_t FleetController::CreateFleet(const std::string& name,
                                       FleetFormation formation) {
    Fleet f;
    f.id        = m_nextFleetId++;
    f.name      = name;
    f.formation = formation;
    uint32_t id = f.id;
    m_fleets[id] = std::move(f);
    return id;
}

void FleetController::DestroyFleet(uint32_t fleetId) { m_fleets.erase(fleetId); }

Fleet* FleetController::GetFleet(uint32_t fleetId) {
    auto it = m_fleets.find(fleetId);
    return it != m_fleets.end() ? &it->second : nullptr;
}

std::vector<uint32_t> FleetController::AllFleetIds() const {
    std::vector<uint32_t> ids;
    for (const auto& [id, _] : m_fleets) ids.push_back(id);
    return ids;
}

// ── Ship management ───────────────────────────────────────────────────────────

void FleetController::AddShipToFleet(uint32_t fleetId, const AIShipAgent& ship) {
    auto it = m_fleets.find(fleetId);
    if (it == m_fleets.end()) return;
    it->second.ships.push_back(ship);
    UpdateFormationOffsets(it->second);
}

void FleetController::RemoveShipFromFleet(uint32_t fleetId, uint32_t entityId) {
    auto it = m_fleets.find(fleetId);
    if (it == m_fleets.end()) return;
    auto& ships = it->second.ships;
    ships.erase(std::remove_if(ships.begin(), ships.end(),
        [entityId](const AIShipAgent& s){ return s.entityId == entityId; }),
        ships.end());
    UpdateFormationOffsets(it->second);
}

void FleetController::UpdateShipPosition(uint32_t entityId,
                                          float x, float y, float z) {
    for (auto& [fid, fleet] : m_fleets)
        for (auto& ship : fleet.ships)
            if (ship.entityId == entityId) { ship.posX = x; ship.posY = y; ship.posZ = z; }
}

void FleetController::UpdateShipHP(uint32_t entityId, float hp) {
    for (auto& [fid, fleet] : m_fleets)
        for (auto& ship : fleet.ships)
            if (ship.entityId == entityId) ship.hullHP = hp;
}

// ── Orders ────────────────────────────────────────────────────────────────────

void FleetController::SetPatrolRoute(uint32_t fleetId,
                                      const std::vector<Waypoint>& route, bool loop) {
    auto it = m_fleets.find(fleetId);
    if (it == m_fleets.end()) return;
    it->second.patrolRoute = route;
    it->second.patrolLoop  = loop;
    it->second.patrolIndex = 0;
    for (auto& s : it->second.ships) s.state = FleetAgentState::Patrolling;
}

void FleetController::OrderEscort(uint32_t fleetId, uint32_t targetEntityId,
                                   float radius) {
    auto it = m_fleets.find(fleetId);
    if (it == m_fleets.end()) return;
    it->second.targetEntityId = targetEntityId;
    it->second.escortRadius   = radius;
    for (auto& s : it->second.ships) s.state = FleetAgentState::Escorting;
}

void FleetController::OrderIntercept(uint32_t fleetId, uint32_t targetEntityId) {
    auto it = m_fleets.find(fleetId);
    if (it == m_fleets.end()) return;
    it->second.targetEntityId = targetEntityId;
    for (auto& s : it->second.ships) s.state = FleetAgentState::Engaging;
}

void FleetController::OrderRetreat(uint32_t fleetId, const Waypoint& safeZone) {
    auto it = m_fleets.find(fleetId);
    if (it == m_fleets.end()) return;
    it->second.targetX = safeZone.x;
    it->second.targetY = safeZone.y;
    it->second.targetZ = safeZone.z;
    for (auto& s : it->second.ships) s.state = FleetAgentState::Retreating;
}

void FleetController::OrderIdle(uint32_t fleetId) {
    auto it = m_fleets.find(fleetId);
    if (it == m_fleets.end()) return;
    for (auto& s : it->second.ships) s.state = FleetAgentState::Idle;
}

// ── Tick ─────────────────────────────────────────────────────────────────────

void FleetController::Tick(float dt) {
    for (auto& [id, fleet] : m_fleets) TickFleet(fleet, dt);
}

void FleetController::TickFleet(Fleet& fleet, float dt) {
    if (fleet.ships.empty()) return;
    // Check for retreat condition
    for (auto& ship : fleet.ships) {
        if (ship.maxHullHP > 0 &&
            ship.hullHP / ship.maxHullHP < fleet.retreatHPThreshold &&
            ship.state != FleetAgentState::Retreating) {
            ship.state = FleetAgentState::Retreating;
            if (m_onStateChanged)
                m_onStateChanged(fleet.id, ship.entityId, FleetAgentState::Retreating);
        }
    }
    // State tick
    switch (fleet.ships[0].state) {
    case FleetAgentState::Patrolling: TickPatrol(fleet, dt); break;
    case FleetAgentState::Escorting:
    case FleetAgentState::Engaging:  TickEscort(fleet, dt); break;
    case FleetAgentState::Retreating:
        for (auto& ship : fleet.ships)
            MoveTowards(ship, fleet.targetX, fleet.targetY, fleet.targetZ, dt);
        break;
    default: break;
    }
}

void FleetController::TickPatrol(Fleet& fleet, float dt) {
    if (fleet.patrolRoute.empty()) return;
    const Waypoint& wp = fleet.patrolRoute[fleet.patrolIndex];
    bool arrived = true;
    for (auto& ship : fleet.ships) {
        float tx = wp.x + ship.offsetX;
        float tz = wp.z + ship.offsetZ;
        MoveTowards(ship, tx, wp.y + ship.offsetY, tz, dt);
        if (DistanceTo(ship, wp.x, wp.z) > wp.arrivalRadius) arrived = false;
    }
    if (arrived) {
        size_t next = fleet.patrolIndex + 1;
        if (next >= fleet.patrolRoute.size()) {
            if (fleet.patrolLoop) fleet.patrolIndex = 0;
            // else stay at last waypoint
        } else {
            fleet.patrolIndex = next;
        }
    }
}

void FleetController::TickEscort(Fleet& fleet, float dt) {
    float tx = fleet.targetX, ty = fleet.targetY, tz = fleet.targetZ;
    if (fleet.targetEntityId != 0 && m_posQuery)
        m_posQuery(fleet.targetEntityId, tx, ty, tz);
    fleet.targetX = tx; fleet.targetY = ty; fleet.targetZ = tz;
    for (auto& ship : fleet.ships)
        MoveTowards(ship, tx + ship.offsetX, ty + ship.offsetY, tz + ship.offsetZ, dt);
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void FleetController::SetPositionQuery(PositionQueryFn fn)        { m_posQuery       = std::move(fn); }
void FleetController::SetStateChangedCallback(FleetStateChangedFn fn) { m_onStateChanged = std::move(fn); }
void FleetController::SetDestroyedCallback(FleetDestroyedFn fn)   { m_onDestroyed    = std::move(fn); }
size_t FleetController::FleetCount() const { return m_fleets.size(); }

// ── Private helpers ───────────────────────────────────────────────────────────

void FleetController::UpdateFormationOffsets(Fleet& fleet) {
    constexpr float kSpacing = 20.0f;
    for (size_t i = 0; i < fleet.ships.size(); ++i) {
        auto& s = fleet.ships[i];
        switch (fleet.formation) {
        case FleetFormation::V:
            s.offsetX = (i % 2 == 0 ? 1.0f : -1.0f) * (float)((i + 1) / 2) * kSpacing;
            s.offsetZ = -(float)((i + 1) / 2) * kSpacing;
            break;
        case FleetFormation::Line:
            s.offsetX = 0;
            s.offsetZ = -(float)i * kSpacing;
            break;
        case FleetFormation::Diamond:
            { float ang = (float)i * (360.0f / (float)fleet.ships.size())
                          * (3.14159265f / 180.0f);
              s.offsetX = std::cos(ang) * kSpacing;
              s.offsetZ = std::sin(ang) * kSpacing; }
            break;
        default:
            s.offsetX = (float)(i % 3 - 1) * kSpacing;
            s.offsetZ = -(float)(i / 3) * kSpacing;
            break;
        }
        s.offsetY = 0;
    }
}

float FleetController::DistanceTo(const AIShipAgent& ship, float tx, float tz) const {
    float dx = ship.posX - tx;
    float dz = ship.posZ - tz;
    return std::sqrt(dx*dx + dz*dz);
}

void FleetController::MoveTowards(AIShipAgent& ship,
                                   float tx, float ty, float tz, float dt) const {
    float dx = tx - ship.posX;
    float dy = ty - ship.posY;
    float dz = tz - ship.posZ;
    float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    if (dist < 0.1f) return;
    float move = ship.speed * dt;
    if (move >= dist) { ship.posX = tx; ship.posY = ty; ship.posZ = tz; }
    else {
        float f = move / dist;
        ship.posX += dx * f;
        ship.posY += dy * f;
        ship.posZ += dz * f;
    }
}

} // namespace Runtime
