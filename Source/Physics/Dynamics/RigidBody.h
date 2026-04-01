#pragma once
#include "Core/Math/Vector.h"

namespace NF {

/// @brief Plain-data snapshot of a rigid body's physical state.
struct RigidBodyState {
    Vector3 Position;        ///< World-space position.
    Vector3 Velocity;        ///< Linear velocity (units/s).
    Vector3 AngularVelocity; ///< Angular velocity (radians/s, world-space axis).
    float   Mass{1.f};       ///< Mass in kilograms; must be > 0.
    bool    IsKinematic{false}; ///< If true the body is not driven by forces.
};

/// @brief A rigid body that can have forces applied and be integrated over time.
class RigidBody {
public:
    /// @brief Construct with an initial state.
    explicit RigidBody(const RigidBodyState& state = {}) noexcept;

    /// @brief Accumulate a force (N) to be applied on the next Integrate call.
    void ApplyForce(const Vector3& force) noexcept;

    /// @brief Apply an instantaneous impulse (kg·m/s), changing velocity immediately.
    void ApplyImpulse(const Vector3& impulse) noexcept;

    /// @brief Integrate the accumulated forces over dt seconds (Euler integration).
    void Integrate(float dt) noexcept;

    /// @brief Read-only access to the current physical state.
    [[nodiscard]] const RigidBodyState& GetState() const noexcept { return m_State; }

    /// @brief Direct write access to the state (e.g., for teleportation).
    [[nodiscard]] RigidBodyState& GetState() noexcept { return m_State; }

private:
    RigidBodyState m_State;
    Vector3        m_AccumulatedForce{};
};

} // namespace NF
