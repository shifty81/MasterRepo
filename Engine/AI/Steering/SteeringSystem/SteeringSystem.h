#pragma once
/**
 * @file SteeringSystem.h
 * @brief Reynolds steering behaviours: seek, flee, arrive, pursue, evade, wander, flocking.
 *
 * Features:
 *   - SteeringAgent: id, position, velocity, maxSpeed, maxForce, mass
 *   - Individual behaviours (combinable with weights):
 *       Seek, Flee, Arrive (with deceleration radius), Pursue, Evade
 *       Wander (offset circle), PathFollow (waypoint list)
 *       AvoidObstacle (sphere obstacles)
 *   - Flocking: Separation, Alignment, Cohesion
 *   - BehaviourSet per agent: list of (behaviour, weight) pairs
 *   - Tick: compute weighted steering → apply to velocity → integrate position
 *   - GetSteering(agentId) → computed force this frame
 *   - SetTarget(agentId, worldPos)
 *   - SetTargetAgent(agentId, targetAgentId)
 *   - GetNeighbours(agentId, radius) → list of agent ids
 *
 * Typical usage:
 * @code
 *   SteeringSystem ss;
 *   ss.Init();
 *   SteeringAgentDesc d; d.maxSpeed=5.f; d.maxForce=15.f;
 *   uint32_t a = ss.CreateAgent(d, {0,0,0});
 *   ss.AddBehaviour(a, BehaviourType::Seek, 1.f);
 *   ss.SetTarget(a, {10,0,5});
 *   ss.Tick(dt);
 *   auto pos = ss.GetPosition(a);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

enum class BehaviourType : uint8_t {
    Seek=0, Flee, Arrive, Pursue, Evade, Wander,
    PathFollow, AvoidObstacle,
    Separation, Alignment, Cohesion
};

struct SteeringAgentDesc {
    float maxSpeed   {5.f};
    float maxForce   {15.f};
    float mass       {1.f};
    float radius     {0.5f};      ///< for separation / obstacle avoidance
    float neighbourR {5.f};       ///< radius for flocking neighbours
};

class SteeringSystem {
public:
    SteeringSystem();
    ~SteeringSystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    // Agent lifecycle
    uint32_t CreateAgent(const SteeringAgentDesc& desc, const float pos[3]=nullptr);
    void     DestroyAgent(uint32_t id);
    bool     HasAgent    (uint32_t id) const;

    // Behaviours
    void AddBehaviour   (uint32_t id, BehaviourType bt, float weight=1.f);
    void RemoveBehaviour(uint32_t id, BehaviourType bt);
    void ClearBehaviours(uint32_t id);

    // Targets
    void SetTarget      (uint32_t id, const float worldPos[3]);
    void SetTargetAgent (uint32_t id, uint32_t targetAgentId);
    void SetWaypoints   (uint32_t id, const std::vector<std::vector<float>>& pts);
    void AddObstacle    (const float centre[3], float radius);
    void ClearObstacles ();

    // Wander params
    void SetWanderParams(uint32_t id, float circleRadius=2.f,
                          float circleOffset=4.f, float jitter=0.3f);

    // Arrive params
    void SetArriveRadius(uint32_t id, float slowRadius=3.f);

    // State
    void GetPosition(uint32_t id, float out[3]) const;
    void GetVelocity(uint32_t id, float out[3]) const;
    void GetSteering(uint32_t id, float out[3]) const;

    void SetPosition(uint32_t id, const float pos[3]);
    void SetVelocity(uint32_t id, const float vel[3]);

    std::vector<uint32_t> GetNeighbours(uint32_t id, float radius) const;
    std::vector<uint32_t> GetAll() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
