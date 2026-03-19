#pragma once
#include <cstdint>
#include "Runtime/ECS/ECS.h"

namespace Runtime::Player {

enum class MovementMode { Walking, Flying, Swimming, NoClip };

struct PlayerInput {
    float moveForward = 0.0f;
    float moveRight   = 0.0f;
    float moveUp      = 0.0f;
    float lookYaw     = 0.0f;
    float lookPitch   = 0.0f;
    bool  jump        = false;
    bool  sprint      = false;
    bool  crouch      = false;
    bool  interact    = false;
    bool  attack      = false;
};

class PlayerController {
public:
    explicit PlayerController(Runtime::ECS::EntityID entity);

    void Update(float dt, const PlayerInput& input);

    Runtime::ECS::EntityID GetEntity() const { return m_entity; }

    void         SetMovementMode(MovementMode mode) { m_mode = mode; }
    MovementMode GetMovementMode() const            { return m_mode; }

    float GetMoveSpeed() const    { return m_moveSpeed; }
    void  SetMoveSpeed(float s)   { m_moveSpeed = s; }

    float GetSprintMultiplier() const       { return m_sprintMultiplier; }
    void  SetSprintMultiplier(float s)      { m_sprintMultiplier = s; }

    float GetJumpForce() const              { return m_jumpForce; }
    void  SetJumpForce(float f)             { m_jumpForce = f; }

private:
    Runtime::ECS::EntityID m_entity;
    MovementMode           m_mode            = MovementMode::Walking;
    float                  m_moveSpeed       = 5.0f;
    float                  m_sprintMultiplier = 2.0f;
    float                  m_jumpForce       = 7.0f;
};

} // namespace Runtime::Player
