#include "Physics/Character/CharacterController.h"
#include "Physics/Dynamics/PhysicsWorld.h"

namespace NF {

CharacterController::CharacterController(const Vector3& position) noexcept
    : m_Position(position) {}

void CharacterController::Move(const Vector3& delta) noexcept {
    m_PendingMove += delta;
}

void CharacterController::Jump(float force) noexcept {
    if (m_IsGrounded) {
        m_Velocity.Y += force;
        m_IsGrounded = false;
    }
}

bool CharacterController::IsGrounded() const noexcept {
    return m_IsGrounded;
}

void CharacterController::Update(float dt, PhysicsWorld& /*world*/) {
    // Apply pending horizontal movement.
    m_Position += m_PendingMove;
    m_PendingMove = {0.f, 0.f, 0.f};

    // Apply gravity to vertical velocity.
    if (!m_IsGrounded)
        m_Velocity.Y += kGravity * dt;

    m_Position.Y += m_Velocity.Y * dt;

    // Simple ground plane at Y = 0.
    if (m_Position.Y <= 0.f) {
        m_Position.Y = 0.f;
        m_Velocity.Y = 0.f;
        m_IsGrounded = true;
    } else {
        m_IsGrounded = false;
    }
}

} // namespace NF
