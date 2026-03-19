#include "Runtime/Player/PlayerController.h"
#include "Runtime/Components/Components.h"

namespace Runtime::Player {

PlayerController::PlayerController(Runtime::ECS::EntityID entity)
    : m_entity(entity) {}

void PlayerController::Update(float dt, const PlayerInput& input) {
    // Speed is multiplied by sprint if sprinting and in a mode that allows it.
    float speed = m_moveSpeed;
    if (input.sprint && (m_mode == MovementMode::Walking || m_mode == MovementMode::NoClip))
        speed *= m_sprintMultiplier;

    // Build a local-space movement vector from input axes.
    Engine::Math::Vec3 move{
        input.moveRight   * speed * dt,
        0.0f,
        input.moveForward * speed * dt
    };

    switch (m_mode) {
        case MovementMode::Flying:
        case MovementMode::NoClip:
            move.y = input.moveUp * speed * dt;
            break;
        case MovementMode::Swimming:
            move.y = input.moveUp * speed * 0.5f * dt;
            break;
        case MovementMode::Walking:
            // Vertical handled by physics/jump below.
            break;
    }

    // Integration point: apply move to the entity's Transform via the ECS World.
    // In a full implementation, obtain World reference or use a command buffer:
    //   auto* xform = world.GetComponent<Runtime::Components::Transform>(m_entity);
    //   if (xform) xform->position += move;
    // We keep this as a stub integration point; callers own the World.
    (void)move;
    (void)dt;

    // Jump impulse is applied through the RigidBody when grounded.
    // Integration point:
    //   if (input.jump && isGrounded) {
    //       auto* rb = world.GetComponent<Runtime::Components::RigidBody>(m_entity);
    //       if (rb) rb->velocity.y = m_jumpForce;
    //   }
}

} // namespace Runtime::Player
