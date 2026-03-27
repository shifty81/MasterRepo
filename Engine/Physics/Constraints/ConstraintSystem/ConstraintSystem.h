#pragma once
/**
 * @file ConstraintSystem.h
 * @brief Runtime physics constraints: distance, hinge, ball, slider, weld, spring.
 *
 * Features:
 *   - CreateConstraint(type, bodyA, bodyB, params) → constraintId
 *   - DestroyConstraint(id)
 *   - SetEnabled(id, enabled) / IsEnabled(id) → bool
 *   - SolveVelocity(dt): velocity-level iteration pass (sequential impulse)
 *   - SolvePosition(dt): position-correction pass (Baumgarte/pseudo-velocity)
 *   - SetIterations(n): solver iteration count
 *   - SetBreakForce(id, maxForce): constraint breaks if impulse exceeds threshold
 *   - GetAppliedForce(id) → float: magnitude of last impulse applied
 *   - SetOnBreak(id, cb): callback when constraint breaks
 *   - SetLimits(id, minAngle, maxAngle): angular limits for hinge/ball
 *   - SetSpring(id, stiffness, damping): spring parameters for spring constraint
 *   - GetConstraintCount() → uint32_t
 *   - Reset() / Shutdown()
 *
 * Body state is owned externally; callers supply position/velocity arrays via
 * SetBodyState(bodyId, pos, vel, invMass) before each Solve call.
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class ConstraintType : uint8_t {
    Distance, Hinge, Ball, Slider, Weld, Spring
};

struct CSVec3 { float x,y,z; };

struct ConstraintParams {
    CSVec3  anchorA{0,0,0};   ///< local offset on body A
    CSVec3  anchorB{0,0,0};   ///< local offset on body B
    CSVec3  axisA  {0,1,0};   ///< constraint axis for hinge/slider
    float   restLength{1.f};  ///< distance / slider rest length
    float   stiffness {0.f};  ///< spring stiffness (spring type)
    float   damping   {0.f};  ///< spring damping
    float   minLimit  {-180.f};
    float   maxLimit  {180.f};
    float   breakForce{1e10f};
};

struct BodyState {
    CSVec3  pos{0,0,0};
    CSVec3  vel{0,0,0};
    float   invMass{1.f};
};

class ConstraintSystem {
public:
    ConstraintSystem();
    ~ConstraintSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Body state (must be set before Solve)
    void SetBodyState(uint32_t bodyId, const BodyState& state);
    BodyState GetBodyState(uint32_t bodyId) const;

    // Constraint management
    uint32_t CreateConstraint (ConstraintType type, uint32_t bodyA, uint32_t bodyB,
                                const ConstraintParams& params);
    void     DestroyConstraint(uint32_t id);
    void     SetEnabled       (uint32_t id, bool enabled);
    bool     IsEnabled        (uint32_t id) const;

    // Per-constraint configuration
    void  SetBreakForce(uint32_t id, float maxForce);
    void  SetLimits    (uint32_t id, float minAngle, float maxAngle);
    void  SetSpring    (uint32_t id, float stiffness, float damping);
    float GetAppliedForce(uint32_t id) const;

    // Solving
    void SetIterations(uint32_t n);
    void SolveVelocity(float dt);
    void SolvePosition(float dt);

    // Callbacks & queries
    void     SetOnBreak(uint32_t id, std::function<void()> cb);
    uint32_t GetConstraintCount() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
