#pragma once
#include "Core/Math/Vector.h"
#include "Game/Voxel/ChunkMap.h"

namespace NF::Game {

/// @brief First-person player movement with voxel-aware AABB collision.
///
/// Manages the player's world position, velocity, and grounded state.
/// Call @c SetInput() each frame with the desired movement direction and
/// look angles, then @c Update() to integrate physics and resolve collisions
/// against the voxel grid.
///
/// The player is modelled as an axis-aligned bounding box centred
/// horizontally on the position.  The position represents the player's
/// feet; the camera eye is @c kEyeHeight above the position.
class PlayerMovement {
public:
    // ---- Tuning constants ---------------------------------------------------

    static constexpr float kMoveSpeed    = 6.f;    ///< Walk speed (m/s).
    static constexpr float kSprintMul    = 1.8f;   ///< Sprint multiplier on walk speed.
    static constexpr float kJumpImpulse  = 7.5f;   ///< Jump velocity (m/s upward).
    static constexpr float kGravity      = 20.f;   ///< Gravitational acceleration (m/s²).
    static constexpr float kEyeHeight    = 1.62f;  ///< Camera above feet position.
    static constexpr float kPlayerWidth  = 0.6f;   ///< Player AABB half-extent XZ.
    static constexpr float kPlayerHeight = 1.8f;   ///< Player AABB full height.
    static constexpr float kMouseSens    = 0.003f;  ///< Radians per mouse pixel.
    static constexpr float kMaxPitch     = 1.5f;   ///< ~86° — avoids gimbal lock.

    // ---- Construction -------------------------------------------------------

    explicit PlayerMovement(const NF::Vector3& spawnPos = {0.f, 12.f, 0.f}) noexcept;

    // ---- Per-frame input ----------------------------------------------------

    /// @brief Set the raw movement input for this frame.
    ///
    /// @param forward  Forward/back in [-1, 1] (W = +1, S = -1).
    /// @param right    Strafe in [-1, 1] (D = +1, A = -1).
    /// @param jump     True on the frame the jump key is pressed.
    /// @param sprint   True while the sprint key is held.
    void SetMoveInput(float forward, float right, bool jump, bool sprint) noexcept;

    /// @brief Apply mouse delta to update the look direction.
    ///
    /// @param dx  Horizontal mouse delta (pixels).
    /// @param dy  Vertical mouse delta (pixels).
    void ApplyMouseLook(float dx, float dy) noexcept;

    // ---- Simulation ---------------------------------------------------------

    /// @brief Integrate velocity, apply gravity, and resolve voxel collisions.
    ///
    /// @param dt   Delta time in seconds.
    /// @param map  Voxel map used for collision queries.
    void Update(float dt, const ChunkMap& map) noexcept;

    // ---- Accessors ----------------------------------------------------------

    /// @brief Current feet position in world space.
    [[nodiscard]] const NF::Vector3& GetPosition() const noexcept { return m_Position; }

    /// @brief Camera eye position (feet + eye height).
    [[nodiscard]] NF::Vector3 GetEyePosition() const noexcept {
        return {m_Position.X, m_Position.Y + kEyeHeight, m_Position.Z};
    }

    /// @brief Current yaw angle (radians, 0 = +Z).
    [[nodiscard]] float GetYaw()   const noexcept { return m_Yaw; }

    /// @brief Current pitch angle (radians, clamped).
    [[nodiscard]] float GetPitch() const noexcept { return m_Pitch; }

    /// @brief True when the player is resting on a surface.
    [[nodiscard]] bool IsGrounded() const noexcept { return m_Grounded; }

    /// @brief Current vertical velocity (m/s, positive = up).
    [[nodiscard]] float GetVerticalVelocity() const noexcept { return m_VelocityY; }

    /// @brief Set the position directly (e.g. for teleport or reset).
    void SetPosition(const NF::Vector3& pos) noexcept { m_Position = pos; m_VelocityY = 0.f; m_Grounded = false; }

    /// @brief Unit-length forward vector derived from yaw (XZ plane).
    [[nodiscard]] NF::Vector3 GetForwardXZ() const noexcept;

    /// @brief Unit-length right vector derived from yaw (XZ plane).
    [[nodiscard]] NF::Vector3 GetRightXZ() const noexcept;

    /// @brief Full view direction incorporating pitch and yaw.
    [[nodiscard]] NF::Vector3 GetViewDirection() const noexcept;

private:
    NF::Vector3 m_Position;
    float       m_Yaw{0.f};
    float       m_Pitch{0.f};
    float       m_VelocityY{0.f};
    bool        m_Grounded{false};

    // Per-frame input (consumed in Update).
    float m_InputForward{0.f};
    float m_InputRight{0.f};
    bool  m_InputJump{false};
    bool  m_InputSprint{false};

    /// @brief Resolve AABB vs voxel collisions along a single axis.
    ///
    /// Sweeps the player AABB along the movement vector and stops at the
    /// first solid voxel boundary.
    void ResolveCollisions(const ChunkMap& map) noexcept;
};

} // namespace NF::Game
