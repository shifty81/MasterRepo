#pragma once
#include "Core/Math/Vector.h"

namespace NF {

class PhysicsWorld;

/// @brief Kinematic character controller driven by explicit Move/Jump calls.
class CharacterController {
public:
    /// @brief Construct with an initial world-space position.
    explicit CharacterController(const Vector3& position = {0.f, 0.f, 0.f}) noexcept;

    /// @brief Request a positional delta to be applied on the next Update.
    void Move(const Vector3& delta) noexcept;

    /// @brief Apply an upward velocity impulse when the character is grounded.
    /// @param force  Vertical speed to add (m/s).
    void Jump(float force) noexcept;

    /// @brief Returns true when the character is resting on a surface.
    [[nodiscard]] bool IsGrounded() const noexcept;

    /// @brief Integrate movement, apply gravity, and resolve ground contact.
    void Update(float dt, PhysicsWorld& world);

    /// @brief Current world-space position.
    [[nodiscard]] const Vector3& GetPosition() const noexcept { return m_Position; }

private:
    static constexpr float kGravity{-9.81f};

    Vector3 m_Position;
    Vector3 m_Velocity{};
    Vector3 m_PendingMove{};
    bool    m_IsGrounded{false};
};

} // namespace NF
