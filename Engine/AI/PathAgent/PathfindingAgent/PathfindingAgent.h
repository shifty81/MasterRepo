#pragma once
/**
 * @file PathfindingAgent.h
 * @brief Navigation agent with arrive/flee/wander/follow steering behaviours using a path provider.
 *
 * Features:
 *   - CreateAgent(id, radius, maxSpeed): register a navigating agent
 *   - RemoveAgent(id)
 *   - SetPath(agentId, waypoints[]): provide a list of world-space path waypoints
 *   - SetTarget(agentId, pos): request a new path (stored; user calls SetPath with result)
 *   - SetBehaviour(agentId, mode): Arrive / Flee / Wander / Idle / Follow
 *   - Update(agentId, currentPos, dt) → newPos: advance agent along path
 *   - GetDesiredVelocity(agentId) → Vec3: movement vector for this frame
 *   - GetCurrentWaypoint(agentId) → Vec3
 *   - HasReachedTarget(agentId) → bool
 *   - SetArrivalRadius(agentId, r): distance threshold for "arrived"
 *   - SetOnWaypointReached(agentId, cb): callback per waypoint
 *   - SetOnTargetReached(agentId, cb): callback at final waypoint
 *   - GetPathLength(agentId) → float: remaining path distance
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

enum class AgentBehaviour : uint8_t { Idle, Arrive, Flee, Wander, Follow };

struct PAVec3f { float x, y, z; };

class PathfindingAgent {
public:
    PathfindingAgent();
    ~PathfindingAgent();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Agent lifecycle
    void CreateAgent(uint32_t id, float radius=0.5f, float maxSpeed=5.f);
    void RemoveAgent(uint32_t id);

    // Path & target
    void SetPath    (uint32_t agentId, const std::vector<PAVec3f>& waypoints);
    void SetTarget  (uint32_t agentId, PAVec3f targetPos);
    void SetBehaviour(uint32_t agentId, AgentBehaviour mode);

    // Per-frame update
    PAVec3f Update(uint32_t agentId, PAVec3f currentPos, float dt);

    // Query
    PAVec3f  GetDesiredVelocity  (uint32_t agentId) const;
    PAVec3f  GetCurrentWaypoint  (uint32_t agentId) const;
    bool     HasReachedTarget    (uint32_t agentId) const;
    float    GetPathLength       (uint32_t agentId) const;
    uint32_t GetWaypointIndex    (uint32_t agentId) const;

    // Config
    void SetMaxSpeed     (uint32_t agentId, float speed);
    void SetArrivalRadius(uint32_t agentId, float r);

    // Callbacks
    void SetOnWaypointReached(uint32_t agentId, std::function<void(uint32_t wpIdx)> cb);
    void SetOnTargetReached  (uint32_t agentId, std::function<void()> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
