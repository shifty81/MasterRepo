#pragma once
/**
 * @file CrowdSimulation.h
 * @brief Large-scale crowd simulation using Boids-style steering + NavMesh.
 *
 * Manages thousands of agents with:
 *   - Separation, alignment, cohesion flocking forces
 *   - NavMesh-aware goal seeking (integrates with Engine/Pathfinding/NavMesh)
 *   - Obstacle / collision avoidance (ORCA-like velocity obstacles)
 *   - Formation groups
 *   - LOD: full physics for near agents, simplified for distant ones
 *
 * Typical usage:
 * @code
 *   CrowdSimulation crowd;
 *   CrowdConfig cfg;
 *   cfg.maxAgents = 2000;
 *   crowd.Init(cfg);
 *   crowd.SetNavMeshPtr(navMeshPtr);
 *   uint32_t id = crowd.SpawnAgent({0,0,0}, {100,0,50});
 *   crowd.Update(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

// ── Configuration ─────────────────────────────────────────────────────────────

struct CrowdConfig {
    uint32_t maxAgents{1000};
    float    agentRadius{0.4f};
    float    agentSpeed{3.5f};          ///< max m/s
    float    separationWeight{1.5f};
    float    alignmentWeight{1.0f};
    float    cohesionWeight{0.8f};
    float    obstacleAvoidWeight{2.0f};
    float    lodNearDistance{30.f};     ///< full physics below this
    float    lodFarDistance{150.f};     ///< disabled beyond this
};

// ── Agent state ───────────────────────────────────────────────────────────────

struct CrowdAgent {
    uint32_t id{0};
    float    position[3]{};
    float    velocity[3]{};
    float    goalPosition[3]{};
    float    radius{0.4f};
    float    speed{3.5f};
    bool     active{true};
    bool     reachedGoal{false};
    uint32_t formationGroupId{0};   ///< 0 = no formation
};

// ── Formation group ───────────────────────────────────────────────────────────

struct CrowdFormation {
    uint32_t             id{0};
    std::string          name;
    float                anchor[3]{};   ///< group leader position
    std::vector<uint32_t> agentIds;
};

// ── CrowdSimulation ───────────────────────────────────────────────────────────

class CrowdSimulation {
public:
    CrowdSimulation();
    ~CrowdSimulation();

    void Init(const CrowdConfig& config = {});
    void Shutdown();

    // ── NavMesh wiring ────────────────────────────────────────────────────────

    void SetNavMeshPtr(void* navMesh);  ///< pass Engine::NavMesh* cast to void*

    // ── Agent management ──────────────────────────────────────────────────────

    uint32_t SpawnAgent(const float position[3], const float goal[3]);
    void     DestroyAgent(uint32_t agentId);
    void     SetAgentGoal(uint32_t agentId, const float goal[3]);
    void     SetAgentSpeed(uint32_t agentId, float speed);

    CrowdAgent GetAgent(uint32_t agentId) const;
    std::vector<CrowdAgent> AllAgents() const;
    uint32_t   ActiveAgentCount() const;

    // ── Formation ─────────────────────────────────────────────────────────────

    uint32_t CreateFormation(const std::string& name);
    void     AddAgentToFormation(uint32_t formationId, uint32_t agentId);
    void     SetFormationGoal(uint32_t formationId, const float goal[3]);
    void     DisbandFormation(uint32_t formationId);

    // ── Simulation ────────────────────────────────────────────────────────────

    void Update(float dt);

    // ── Spatial query ─────────────────────────────────────────────────────────

    std::vector<uint32_t> AgentsInRadius(const float centre[3],
                                          float radius) const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnAgentGoalReached(std::function<void(uint32_t agentId)> cb);
    void OnAgentBlocked(std::function<void(uint32_t agentId)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
