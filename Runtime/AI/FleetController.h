#pragma once
/**
 * @file FleetController.h
 * @brief AI fleet intelligence — formation flying, escort/intercept, patrol routes.
 *
 * Manages groups of NPC ships (fleets) with a shared commander.
 * Each ship in a fleet is an AIShipAgent with a state machine:
 *
 *   Idle → Patrol → Engage → Retreat → Regroup
 *
 * Supports:
 *   - Formation types: V, Line, Diamond, Sphere
 *   - Patrol: waypoint list; loops or one-shot
 *   - Escort: stay within escort radius of a target entity
 *   - Intercept: pursue and attack a hostile entity
 *   - Retreat: move to a safe zone when below HP threshold
 *
 * Usage:
 * @code
 *   FleetController fc;
 *   fc.Init();
 *   uint32_t fleet = fc.CreateFleet("Patrol Wing Alpha", FleetFormation::V);
 *   fc.AddShipToFleet(fleet, {shipEntity, 0.0f, 0.0f, 0.0f, 200.0f, 200.0f});
 *   fc.SetPatrolRoute(fleet, {{0,0,0},{100,0,0},{100,0,100}});
 *   fc.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Runtime {

// ── Formation types ───────────────────────────────────────────────────────────

enum class FleetFormation : uint8_t {
    V,           ///< wedge / arrowhead
    Line,        ///< single-file
    Diamond,     ///< 4 ships at cardinal offsets
    Sphere,      ///< 3D surrounding formation
    Scattered,   ///< loose offset positions
};

// ── AI ship agent state ───────────────────────────────────────────────────────

enum class FleetAgentState : uint8_t {
    Idle,
    Patrolling,
    Escorting,
    Engaging,
    Retreating,
    Regrouping,
};

struct AIShipAgent {
    uint32_t entityId{0};
    float    posX{0}, posY{0}, posZ{0};
    float    hullHP{200.0f}, maxHullHP{200.0f};
    float    speed{30.0f};
    uint32_t factionId{0};
    FleetAgentState state{FleetAgentState::Idle};
    // Formation offset from fleet commander
    float    offsetX{0}, offsetY{0}, offsetZ{0};
};

// ── Waypoint ──────────────────────────────────────────────────────────────────

struct Waypoint {
    float x{0}, y{0}, z{0};
    float arrivalRadius{10.0f};
};

// ── Fleet ─────────────────────────────────────────────────────────────────────

struct Fleet {
    uint32_t               id{0};
    std::string            name;
    FleetFormation         formation{FleetFormation::V};
    std::vector<AIShipAgent> ships;
    uint32_t               commanderIndex{0};

    // Patrol
    std::vector<Waypoint>  patrolRoute;
    size_t                 patrolIndex{0};
    bool                   patrolLoop{true};

    // Escort / intercept target
    uint32_t               targetEntityId{0};
    float                  targetX{0}, targetY{0}, targetZ{0};
    float                  escortRadius{50.0f};

    float                  retreatHPThreshold{0.25f}; ///< fraction of max HP
};

// ── Fleet events ──────────────────────────────────────────────────────────────

using FleetStateChangedFn = std::function<void(uint32_t fleetId, uint32_t shipEntityId,
                                                FleetAgentState newState)>;
using FleetDestroyedFn    = std::function<void(uint32_t fleetId)>;

// ── FleetController ───────────────────────────────────────────────────────────

class FleetController {
public:
    void Init();

    // ── Fleet management ──────────────────────────────────────────────────
    uint32_t CreateFleet(const std::string& name,
                          FleetFormation formation = FleetFormation::V);
    void     DestroyFleet(uint32_t fleetId);
    Fleet*   GetFleet(uint32_t fleetId);
    std::vector<uint32_t> AllFleetIds() const;

    // ── Ship management ───────────────────────────────────────────────────
    void AddShipToFleet(uint32_t fleetId, const AIShipAgent& ship);
    void RemoveShipFromFleet(uint32_t fleetId, uint32_t entityId);
    void UpdateShipPosition(uint32_t entityId, float x, float y, float z);
    void UpdateShipHP(uint32_t entityId, float hp);

    // ── Orders ────────────────────────────────────────────────────────────
    void SetPatrolRoute(uint32_t fleetId, const std::vector<Waypoint>& route,
                         bool loop = true);
    void OrderEscort(uint32_t fleetId, uint32_t targetEntityId, float radius = 50.0f);
    void OrderIntercept(uint32_t fleetId, uint32_t targetEntityId);
    void OrderRetreat(uint32_t fleetId, const Waypoint& safeZone);
    void OrderIdle(uint32_t fleetId);

    // ── Tick ──────────────────────────────────────────────────────────────
    void Tick(float dt);

    // ── Target position callback ─────────────────────────────────────────
    /// Supply a function that resolves an entityId to (x,y,z).
    using PositionQueryFn = std::function<bool(uint32_t entityId,
                                                float& x, float& y, float& z)>;
    void SetPositionQuery(PositionQueryFn fn);

    // ── Callbacks ─────────────────────────────────────────────────────────
    void SetStateChangedCallback(FleetStateChangedFn fn);
    void SetDestroyedCallback(FleetDestroyedFn fn);

    size_t FleetCount() const;

private:
    void TickFleet(Fleet& fleet, float dt);
    void TickPatrol(Fleet& fleet, float dt);
    void TickEscort(Fleet& fleet, float dt);
    void TickEngage(Fleet& fleet, float dt);
    void UpdateFormationOffsets(Fleet& fleet);
    float DistanceTo(const AIShipAgent& ship, float tx, float tz) const;
    void MoveTowards(AIShipAgent& ship, float tx, float ty, float tz, float dt) const;

    uint32_t m_nextFleetId{1};
    std::unordered_map<uint32_t, Fleet> m_fleets;
    PositionQueryFn     m_posQuery;
    FleetStateChangedFn m_onStateChanged;
    FleetDestroyedFn    m_onDestroyed;
};

} // namespace Runtime
