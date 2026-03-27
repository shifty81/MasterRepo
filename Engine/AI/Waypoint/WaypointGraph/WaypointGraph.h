#pragma once
/**
 * @file WaypointGraph.h
 * @brief Graph of 3-D waypoints for NPC navigation: add, connect, Dijkstra/A*.
 *
 * Features:
 *   - AddWaypoint(id, x, y, z) → bool
 *   - RemoveWaypoint(id)
 *   - ConnectWaypoints(idA, idB, bidirectional = true) → bool
 *   - DisconnectWaypoints(idA, idB)
 *   - FindPath(startId, goalId, outPath[]) → bool: Dijkstra
 *   - FindPathAStar(startId, goalId, outPath[]) → bool: A* with Euclidean heuristic
 *   - GetNeighbours(id, out[]) → uint32_t
 *   - GetNearestWaypoint(x, y, z) → uint32_t
 *   - GetWaypointCount() → uint32_t
 *   - GetWaypointPosition(id, outX, outY, outZ)
 *   - SetEdgeCost(idA, idB, cost)
 *   - GetEdgeCost(idA, idB) → float
 *   - SetEnabled(id, on): disable a waypoint (blocks pathfinding)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <vector>

namespace Engine {

class WaypointGraph {
public:
    WaypointGraph();
    ~WaypointGraph();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Graph construction
    bool AddWaypoint   (uint32_t id, float x, float y, float z);
    void RemoveWaypoint(uint32_t id);
    bool ConnectWaypoints   (uint32_t idA, uint32_t idB,
                             bool bidirectional = true);
    void DisconnectWaypoints(uint32_t idA, uint32_t idB);

    // Edge costs
    void  SetEdgeCost(uint32_t idA, uint32_t idB, float cost);
    float GetEdgeCost(uint32_t idA, uint32_t idB) const;

    // Enable/disable
    void SetEnabled(uint32_t id, bool on);

    // Pathfinding
    bool FindPath     (uint32_t startId, uint32_t goalId,
                       std::vector<uint32_t>& outPath) const;
    bool FindPathAStar(uint32_t startId, uint32_t goalId,
                       std::vector<uint32_t>& outPath) const;

    // Query
    uint32_t GetNeighbours       (uint32_t id, std::vector<uint32_t>& out) const;
    uint32_t GetNearestWaypoint  (float x, float y, float z) const;
    uint32_t GetWaypointCount    () const;
    void     GetWaypointPosition (uint32_t id,
                                  float& x, float& y, float& z) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
