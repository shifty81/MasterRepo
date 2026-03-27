#pragma once
/**
 * @file BoidSystem.h
 * @brief 3D boid flock: cohesion/separation/alignment, boundaries, max-speed.
 *
 * Features:
 *   - Boid: id, position, velocity, mass, radius
 *   - Per-flock config: neighbourRadius, cohesionWeight, separationWeight, alignmentWeight
 *   - maxSpeed, maxForce, damping
 *   - Boundary: axis-aligned box — soft repulsion force near walls
 *   - Obstacle avoidance: sphere obstacles, ray-ahead probe
 *   - Tick: compute forces per boid → integrate velocity/position (Euler)
 *   - Goal: optional attractor world position with weight
 *   - Multiple independent flocks
 *   - GetBoidPos / GetBoidVel
 *   - Per-flock spatial partition (flat grid) for O(n) neighbour query
 *
 * Typical usage:
 * @code
 *   BoidSystem bs;
 *   bs.Init();
 *   BoidFlockDesc fd; fd.count=200; fd.neighbourRadius=3.f; fd.maxSpeed=5.f;
 *   uint32_t flock = bs.CreateFlock(fd, {0,0,0}, 50.f);
 *   bs.SetGoal(flock, {20,5,10}, 0.5f);
 *   bs.Tick(dt);
 *   for (uint32_t i=0; i<200; i++) auto pos = bs.GetBoidPos(flock, i);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct BoidFlockDesc {
    uint32_t count          {100};
    float    neighbourRadius{3.f};
    float    separationDist {0.8f};
    float    cohesionWeight {1.f};
    float    separationWeight{1.5f};
    float    alignmentWeight{1.f};
    float    maxSpeed       {5.f};
    float    maxForce       {8.f};
    float    mass           {1.f};
    float    damping        {0.05f};
    float    boundaryHalfExtent{30.f};  ///< soft boundary cube
    float    boundaryRepulsion {5.f};
};

class BoidSystem {
public:
    BoidSystem();
    ~BoidSystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    // Flock lifecycle
    uint32_t CreateFlock (const BoidFlockDesc& desc,
                           const float origin[3]=nullptr, float spread=5.f);
    void     DestroyFlock(uint32_t flockId);
    bool     HasFlock    (uint32_t flockId) const;

    // Goal
    void SetGoal   (uint32_t flockId, const float worldPos[3], float weight=0.5f);
    void ClearGoal (uint32_t flockId);

    // Obstacles
    void AddObstacle   (uint32_t flockId, const float centre[3], float radius);
    void ClearObstacles(uint32_t flockId);

    // Boundary
    void SetBoundary(uint32_t flockId, const float centre[3], float halfExtent);

    // Weights (runtime adjust)
    void SetWeights(uint32_t flockId, float cohesion, float separation, float alignment);

    // Boid access
    uint32_t     BoidCount    (uint32_t flockId) const;
    const float* GetBoidPos   (uint32_t flockId, uint32_t boidIdx) const;  ///< ptr [3]
    const float* GetBoidVel   (uint32_t flockId, uint32_t boidIdx) const;  ///< ptr [3]

    std::vector<uint32_t> GetAllFlocks() const;

    // Spawn/despawn individual boids
    void SpawnBoid (uint32_t flockId, const float pos[3], const float vel[3]=nullptr);
    void DespawnBoid(uint32_t flockId, uint32_t boidIdx);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
