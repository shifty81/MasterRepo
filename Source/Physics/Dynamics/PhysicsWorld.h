#pragma once
#include "Physics/Dynamics/RigidBody.h"
#include "Physics/Collision/CollisionDetection.h"
#include <vector>

namespace NF {

/// @brief Owns a collection of RigidBody pointers and advances the simulation.
class PhysicsWorld {
public:
    /// @brief Register a body with this world (does not take ownership).
    void AddBody(RigidBody* body);

    /// @brief Unregister a body; safe to call if the body is not present.
    void RemoveBody(RigidBody* body);

    /// @brief Advance all non-kinematic bodies by dt seconds.
    void Step(float dt);

    /// @brief Cast a ray against all body bounds and report the nearest hit.
    /// @param origin   Ray origin in world space.
    /// @param dir      Unit-length ray direction.
    /// @param maxDist  Maximum query distance.
    /// @param hit      Populated with the nearest hit when returning true.
    /// @return True if any body was hit within maxDist.
    [[nodiscard]] bool Raycast(const Vector3& origin, const Vector3& dir,
                               float maxDist, RaycastHit& hit) const;

private:
    std::vector<RigidBody*> m_Bodies;
};

} // namespace NF
