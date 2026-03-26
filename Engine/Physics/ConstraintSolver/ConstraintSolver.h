#pragma once
/**
 * @file ConstraintSolver.h
 * @brief Sequential impulse constraint solver for rigid-body physics.
 *
 * Solves distance, hinge, point, motor, slider, and spring constraints using
 * an iterative impulse method (PGS).  Designed to integrate alongside the
 * existing engine physics pipeline.
 *
 * Features:
 *   - Distance constraint (fixed length rod / rope)
 *   - Hinge (revolute) — axis-aligned, with optional limits
 *   - Slider (prismatic) — translational, with limits
 *   - Spring — soft distance with stiffness/damping
 *   - Point-to-point (ball joint)
 *   - Motor — angular velocity target on hinge
 *   - Solver iterations configurable per solve
 *
 * Typical usage:
 * @code
 *   ConstraintSolver cs;
 *   cs.Init();
 *   uint32_t id = cs.AddDistance(bodyA, bodyB, restLength);
 *   cs.Solve(dt);
 * @endcode
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Engine {

enum class ConstraintType : uint8_t {
    Distance=0, PointToPoint, Hinge, Slider, Spring, Motor, Fixed
};

struct RigidBodyState {
    uint32_t id{0};
    float    position[3]{};
    float    velocity[3]{};
    float    angularVelocity[3]{};
    float    inverseMass{1.f};      ///< 0 = static
    float    inertiaTensor[9]{};    ///< 3×3 world-space
};

struct ConstraintDesc {
    ConstraintType type{ConstraintType::Distance};
    uint32_t       bodyA{0};
    uint32_t       bodyB{0};          ///< 0 = fixed anchor
    float          anchorA[3]{};
    float          anchorB[3]{};
    float          axis[3]{0,1,0};    ///< hinge / slider axis
    float          restLength{1.f};
    float          stiffness{100.f};
    float          damping{0.1f};
    float          limitMin{-3.14159f};
    float          limitMax{ 3.14159f};
    float          motorSpeed{0.f};
    float          motorTorque{0.f};
    bool           limitEnabled{false};
    bool           motorEnabled{false};
};

struct ConstraintEntry {
    uint32_t     id{0};
    ConstraintDesc desc;
    float        lambda{0.f};         ///< accumulated impulse
    bool         enabled{true};
};

class ConstraintSolver {
public:
    ConstraintSolver();
    ~ConstraintSolver();

    void Init(uint32_t iterations = 10);
    void Shutdown();

    // Constraint management
    uint32_t AddConstraint(const ConstraintDesc& desc);
    void     RemoveConstraint(uint32_t id);
    void     EnableConstraint(uint32_t id, bool enabled);
    ConstraintEntry GetConstraint(uint32_t id) const;
    std::vector<ConstraintEntry> AllConstraints() const;

    // Body state (injected from physics engine each step)
    void SetBodyState(const RigidBodyState& state);
    RigidBodyState GetBodyState(uint32_t bodyId) const;
    void RemoveBody(uint32_t bodyId);

    // Solve
    void Solve(float dt);

    // Post-solve: retrieve corrected velocities/positions for body
    RigidBodyState GetCorrectedState(uint32_t bodyId) const;

    void SetIterations(uint32_t n);
    uint32_t GetIterations() const;

    void Clear();

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
