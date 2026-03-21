#pragma once
/**
 * @file PhysicsWorld.h
 * @brief Simple AABB rigid-body physics simulation.
 *
 * PhysicsWorld simulates a set of RigidBodies using axis-aligned bounding boxes:
 *   - BodyType: Dynamic (affected by gravity + impulses), Kinematic (velocity-driven,
 *     no gravity, no impulse reception), Static (immovable collider)
 *   - Broad-phase: uniform spatial grid partitioning; only neighbouring cells tested.
 *   - Narrow-phase: AABB overlap test; penetration axis computed via minimum overlap.
 *   - Resolution: impulse-based with restitution (bounciness) + friction.
 *   - Callbacks: OnCollision fired once per colliding pair per frame.
 *   - Step(dt): advance simulation by fixed timestep; substep support via SubSteps.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include "Engine/Math/Math.h"

namespace Runtime {

// Forward-declare the full namespace alias for convenience
using Vec3 = Engine::Math::Vec3;

// ── Body type ─────────────────────────────────────────────────────────────
enum class BodyType : uint8_t { Static, Kinematic, Dynamic };

// ── Rigid body ────────────────────────────────────────────────────────────
struct RigidBody {
    uint32_t   id{0};
    std::string tag;               ///< User-assigned label
    BodyType   type{BodyType::Dynamic};

    // Transform
    Vec3 position{};
    Vec3 halfExtents{0.5f,0.5f,0.5f};  ///< Half-size of AABB
    // Dynamics
    Vec3 velocity{};
    Vec3 force{};            ///< Accumulated per-frame forces
    float      mass{1.0f};        ///< kg; ignored for Static/Kinematic
    float      restitution{0.3f}; ///< [0,1] bounciness
    float      friction{0.5f};    ///< [0,1] friction coefficient
    bool       sleeping{false};

    // Computed
    Vec3 minBound() const { return {position.x-halfExtents.x,
                                           position.y-halfExtents.y,
                                           position.z-halfExtents.z}; }
    Vec3 maxBound() const { return {position.x+halfExtents.x,
                                           position.y+halfExtents.y,
                                           position.z+halfExtents.z}; }
};

// ── Collision event ────────────────────────────────────────────────────────
struct CollisionEvent {
    uint32_t  bodyA{0};
    uint32_t  bodyB{0};
    Vec3 normal{};        ///< Collision normal pointing from A to B
    float      penetration{0};  ///< Overlap depth
};

// ── Collision callback ─────────────────────────────────────────────────────
using CollisionCb = std::function<void(const CollisionEvent&)>;

// ── Physics config ─────────────────────────────────────────────────────────
struct PhysicsConfig {
    Vec3 gravity{0.0f, -9.81f, 0.0f};
    float      sleepThreshold{0.01f};  ///< Velocity below which bodies sleep
    uint32_t   subSteps{2};            ///< Substeps per Step(dt) call
    float      cellSize{4.0f};         ///< Spatial grid cell size
};

// ── World ─────────────────────────────────────────────────────────────────
class PhysicsWorld {
public:
    PhysicsWorld();
    explicit PhysicsWorld(const PhysicsConfig& cfg);
    ~PhysicsWorld();

    // ── body management ───────────────────────────────────────
    uint32_t AddBody(const RigidBody& proto);
    bool     RemoveBody(uint32_t id);
    RigidBody*       GetBody(uint32_t id);
    const RigidBody* GetBody(uint32_t id) const;
    std::vector<uint32_t> BodyIDs() const;
    void Clear();

    // ── forces / impulses ─────────────────────────────────────
    void ApplyForce(uint32_t id, const Vec3& force);
    void ApplyImpulse(uint32_t id, const Vec3& impulse);
    void SetVelocity(uint32_t id, const Vec3& vel);

    // ── simulation ────────────────────────────────────────────
    void Step(float dt);
    uint64_t FrameCount() const;

    // ── query ─────────────────────────────────────────────────
    /// Returns all bodies whose AABB overlaps the given AABB.
    std::vector<uint32_t> QueryAABB(const Vec3& min,
                                     const Vec3& max) const;
    /// Ray-cast against all dynamic/static bodies; returns nearest hit id or 0.
    uint32_t RayCast(const Vec3& origin,
                     const Vec3& direction,
                     float maxDist,
                     float& hitDist) const;

    // ── callbacks ─────────────────────────────────────────────
    void OnCollision(CollisionCb cb);

    // ── config ────────────────────────────────────────────────
    void SetConfig(const PhysicsConfig& cfg);
    const PhysicsConfig& Config() const;

    // ── stats ─────────────────────────────────────────────────
    uint32_t CollisionCount() const;  ///< Collisions detected in last Step()

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
