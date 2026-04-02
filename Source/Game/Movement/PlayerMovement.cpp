#include "Game/Movement/PlayerMovement.h"
#include <cmath>
#include <algorithm>

namespace NF::Game {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

PlayerMovement::PlayerMovement(const NF::Vector3& spawnPos) noexcept
    : m_Position(spawnPos) {}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void PlayerMovement::SetMoveInput(float forward, float right,
                                   bool jump, bool sprint) noexcept
{
    m_InputForward = std::clamp(forward, -1.f, 1.f);
    m_InputRight   = std::clamp(right,   -1.f, 1.f);
    m_InputJump    = jump;
    m_InputSprint  = sprint;
}

void PlayerMovement::ApplyMouseLook(float dx, float dy) noexcept
{
    m_Yaw   += dx * kMouseSens;
    m_Pitch -= dy * kMouseSens;
    m_Pitch  = std::clamp(m_Pitch, -kMaxPitch, kMaxPitch);
}

// ---------------------------------------------------------------------------
// Direction helpers
// ---------------------------------------------------------------------------

NF::Vector3 PlayerMovement::GetForwardXZ() const noexcept
{
    return NF::Vector3{std::sin(m_Yaw), 0.f, std::cos(m_Yaw)};
}

NF::Vector3 PlayerMovement::GetRightXZ() const noexcept
{
    return NF::Vector3{std::cos(m_Yaw), 0.f, -std::sin(m_Yaw)};
}

NF::Vector3 PlayerMovement::GetViewDirection() const noexcept
{
    const float cp = std::cos(m_Pitch);
    return NF::Vector3{
        cp * std::sin(m_Yaw),
        std::sin(m_Pitch),
        cp * std::cos(m_Yaw)
    };
}

// ---------------------------------------------------------------------------
// Collision resolution
// ---------------------------------------------------------------------------

void PlayerMovement::ResolveCollisions(const ChunkMap& map) noexcept
{
    // Player AABB: position is feet-centre.
    // Half-extents: kPlayerWidth on X/Z, kPlayerHeight on Y (from feet).
    constexpr float hw = kPlayerWidth * 0.5f;
    constexpr float h  = kPlayerHeight;

    // Check a grid of voxels around the player AABB for penetration.
    // We check each solid voxel that overlaps and push the player out
    // along the axis with the smallest overlap.

    const float minX = m_Position.X - hw;
    const float maxX = m_Position.X + hw;
    const float minY = m_Position.Y;
    const float maxY = m_Position.Y + h;
    const float minZ = m_Position.Z - hw;
    const float maxZ = m_Position.Z + hw;

    const int32_t vMinX = static_cast<int32_t>(std::floor(minX));
    const int32_t vMaxX = static_cast<int32_t>(std::floor(maxX));
    const int32_t vMinY = static_cast<int32_t>(std::floor(minY));
    const int32_t vMaxY = static_cast<int32_t>(std::floor(maxY));
    const int32_t vMinZ = static_cast<int32_t>(std::floor(minZ));
    const int32_t vMaxZ = static_cast<int32_t>(std::floor(maxZ));

    for (int32_t vy = vMinY; vy <= vMaxY; ++vy) {
        for (int32_t vx = vMinX; vx <= vMaxX; ++vx) {
            for (int32_t vz = vMinZ; vz <= vMaxZ; ++vz) {
                if (!map.IsSolidAt(vx, vy, vz, false))
                    continue;

                // Voxel AABB is [vx, vx+1] × [vy, vy+1] × [vz, vz+1].
                const float bMinX = static_cast<float>(vx);
                const float bMaxX = bMinX + 1.f;
                const float bMinY = static_cast<float>(vy);
                const float bMaxY = bMinY + 1.f;
                const float bMinZ = static_cast<float>(vz);
                const float bMaxZ = bMinZ + 1.f;

                // Compute overlap on each axis.
                const float overlapPosX = bMaxX - (m_Position.X - hw);
                const float overlapNegX = (m_Position.X + hw) - bMinX;
                const float overlapPosY = bMaxY - m_Position.Y;
                const float overlapNegY = (m_Position.Y + h) - bMinY;
                const float overlapPosZ = bMaxZ - (m_Position.Z - hw);
                const float overlapNegZ = (m_Position.Z + hw) - bMinZ;

                // All overlaps must be positive for a collision.
                if (overlapPosX <= 0.f || overlapNegX <= 0.f ||
                    overlapPosY <= 0.f || overlapNegY <= 0.f ||
                    overlapPosZ <= 0.f || overlapNegZ <= 0.f)
                    continue;

                // Find minimum penetration axis.
                float minOverlap = overlapPosX;
                int   axis       = 0; // 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z

                if (overlapNegX < minOverlap) { minOverlap = overlapNegX; axis = 1; }
                if (overlapPosY < minOverlap) { minOverlap = overlapPosY; axis = 2; }
                if (overlapNegY < minOverlap) { minOverlap = overlapNegY; axis = 3; }
                if (overlapPosZ < minOverlap) { minOverlap = overlapPosZ; axis = 4; }
                if (overlapNegZ < minOverlap) { minOverlap = overlapNegZ; axis = 5; }

                switch (axis) {
                case 0: m_Position.X += minOverlap; break; // push +X
                case 1: m_Position.X -= minOverlap; break; // push -X
                case 2: // push +Y (landed on top of voxel)
                    m_Position.Y += minOverlap;
                    if (m_VelocityY < 0.f) {
                        m_VelocityY = 0.f;
                        m_Grounded  = true;
                    }
                    break;
                case 3: // push -Y (hit head on ceiling)
                    m_Position.Y -= minOverlap;
                    if (m_VelocityY > 0.f)
                        m_VelocityY = 0.f;
                    break;
                case 4: m_Position.Z += minOverlap; break; // push +Z
                case 5: m_Position.Z -= minOverlap; break; // push -Z
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void PlayerMovement::Update(float dt, const ChunkMap& map) noexcept
{
    // Clamp dt to avoid physics explosion on frame spikes.
    dt = std::min(dt, 0.05f);

    // --- Horizontal movement ---
    const NF::Vector3 fwd   = GetForwardXZ();
    const NF::Vector3 right = GetRightXZ();
    const float speed = kMoveSpeed * (m_InputSprint ? kSprintMul : 1.f);

    NF::Vector3 wish{
        fwd.X * m_InputForward + right.X * m_InputRight,
        0.f,
        fwd.Z * m_InputForward + right.Z * m_InputRight
    };

    // Normalise wish direction to avoid faster diagonal movement.
    const float wishLen = std::sqrt(wish.X * wish.X + wish.Z * wish.Z);
    if (wishLen > 1e-4f) {
        wish.X /= wishLen;
        wish.Z /= wishLen;
    }

    m_Position.X += wish.X * speed * dt;
    m_Position.Z += wish.Z * speed * dt;

    // --- Vertical movement ---
    if (m_InputJump && m_Grounded) {
        m_VelocityY = kJumpImpulse;
        m_Grounded  = false;
    }

    m_VelocityY -= kGravity * dt;
    m_Position.Y += m_VelocityY * dt;

    // Reset grounded before collision pass — collision will re-set it.
    m_Grounded = false;

    // --- Collision ---
    ResolveCollisions(map);

    // --- Clear per-frame input ---
    m_InputForward = 0.f;
    m_InputRight   = 0.f;
    m_InputJump    = false;
    m_InputSprint  = false;
}

} // namespace NF::Game
