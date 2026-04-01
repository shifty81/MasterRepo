#pragma once
#include "Core/Math/Vector.h"

namespace NF {

/// @brief Axis-aligned bounding box.
struct AABB {
    Vector3 Min; ///< Minimum corner.
    Vector3 Max; ///< Maximum corner.
};

/// @brief Sphere primitive.
struct Sphere {
    Vector3 Center; ///< Centre position.
    float   Radius; ///< Radius.
};

/// @brief Result of a successful raycast query.
struct RaycastHit {
    Vector3 Point;    ///< World-space hit point.
    Vector3 Normal;   ///< Surface normal at the hit point.
    float   Distance; ///< Distance from ray origin to hit point.
};

/// @brief Static collision-detection utilities.
class CollisionDetection {
public:
    /// @brief Test whether two AABBs overlap.
    [[nodiscard]] static bool TestAABBAABB(const AABB& a, const AABB& b) noexcept;

    /// @brief Test whether two spheres overlap.
    [[nodiscard]] static bool TestSphereSphere(const Sphere& a, const Sphere& b) noexcept;

    /// @brief Cast a ray against an AABB and report the nearest hit.
    /// @param origin   Ray origin in world space.
    /// @param dir      Unit-length ray direction.
    /// @param maxDist  Maximum query distance.
    /// @param aabb     The AABB to test against.
    /// @param hit      Populated with hit data when the function returns true.
    /// @return True if the ray intersects the AABB within maxDist.
    [[nodiscard]] static bool Raycast(const Vector3& origin, const Vector3& dir,
                                      float maxDist, const AABB& aabb,
                                      RaycastHit& hit) noexcept;
};

} // namespace NF
