#pragma once
/**
 * @file CrowdSimulation.h
 * @brief ORCA-based crowd agent simulation: avoidance, goals, steering, density maps.
 *
 * Features:
 *   - AddAgent(id, pos, radius, maxSpeed) → bool
 *   - RemoveAgent(id)
 *   - SetGoal(id, goalPos) / ClearGoal(id)
 *   - SetAgentMaxSpeed(id, v) / SetAgentRadius(id, r)
 *   - GetAgentPos(id) → Vec3
 *   - GetAgentVelocity(id) → Vec3
 *   - GetAgentCount() → uint32_t
 *   - AddStaticObstacle(verts[], count): convex polygon boundary
 *   - ClearStaticObstacles()
 *   - Step(dt): compute ORCA velocities and advance positions
 *   - SetOnAgentArrive(cb): callback(agentId) when agent reaches goal
 *   - GetDensityAt(pos, radius) → float: local agent count density
 *   - SetPreferredVelocity(id, vel): override preferred velocity for one tick
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct CrowdVec3 { float x, y, z; };

class CrowdSimulation {
public:
    CrowdSimulation();
    ~CrowdSimulation();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Agent management
    bool AddAgent   (uint32_t id, CrowdVec3 pos,
                     float radius = 0.4f, float maxSpeed = 3.5f);
    void RemoveAgent(uint32_t id);
    uint32_t GetAgentCount() const;

    // Goals & preferences
    void SetGoal             (uint32_t id, CrowdVec3 goalPos);
    void ClearGoal           (uint32_t id);
    void SetPreferredVelocity(uint32_t id, CrowdVec3 vel);
    void SetAgentMaxSpeed    (uint32_t id, float v);
    void SetAgentRadius      (uint32_t id, float r);

    // Query
    CrowdVec3 GetAgentPos     (uint32_t id) const;
    CrowdVec3 GetAgentVelocity(uint32_t id) const;
    float     GetDensityAt    (CrowdVec3 pos, float radius) const;

    // Obstacles
    void AddStaticObstacle  (const CrowdVec3* verts, uint32_t count);
    void ClearStaticObstacles();

    // Per-frame
    void Step(float dt);

    // Callbacks
    void SetOnAgentArrive(std::function<void(uint32_t agentId)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
