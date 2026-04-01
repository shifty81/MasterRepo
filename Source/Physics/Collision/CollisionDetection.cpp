#include "Physics/Collision/CollisionDetection.h"
#include <cmath>
#include <algorithm>

namespace NF {

bool CollisionDetection::TestAABBAABB(const AABB& a, const AABB& b) noexcept {
    return (a.Min.X <= b.Max.X && a.Max.X >= b.Min.X) &&
           (a.Min.Y <= b.Max.Y && a.Max.Y >= b.Min.Y) &&
           (a.Min.Z <= b.Max.Z && a.Max.Z >= b.Min.Z);
}

bool CollisionDetection::TestSphereSphere(const Sphere& a, const Sphere& b) noexcept {
    const Vector3 diff{a.Center.X - b.Center.X,
                       a.Center.Y - b.Center.Y,
                       a.Center.Z - b.Center.Z};
    const float distSq = diff.Dot(diff);
    const float radSum = a.Radius + b.Radius;
    return distSq <= radSum * radSum;
}

bool CollisionDetection::Raycast(const Vector3& origin, const Vector3& dir,
                                 float maxDist, const AABB& aabb,
                                 RaycastHit& hit) noexcept {
    // Slab method (Kay & Kajiya).
    float tMin = 0.f;
    float tMax = maxDist;

    const float* o = &origin.X;
    const float* d = &dir.X;
    const float* mn = &aabb.Min.X;
    const float* mx = &aabb.Max.X;

    for (int i = 0; i < 3; ++i) {
        if (std::abs(d[i]) < 1e-8f) {
            if (o[i] < mn[i] || o[i] > mx[i])
                return false;
        } else {
            const float inv = 1.f / d[i];
            float t1 = (mn[i] - o[i]) * inv;
            float t2 = (mx[i] - o[i]) * inv;
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            if (tMin > tMax)
                return false;
        }
    }

    hit.Distance = tMin;
    hit.Point = {origin.X + dir.X * tMin,
                 origin.Y + dir.Y * tMin,
                 origin.Z + dir.Z * tMin};

    // Determine the face normal by finding which slab tMin corresponds to.
    hit.Normal = {0.f, 0.f, 0.f};
    const float eps = 1e-4f;
    for (int i = 0; i < 3; ++i) {
        const float face = (&hit.Point.X)[i];
        if (std::abs(face - mn[i]) < eps)      (&hit.Normal.X)[i] = -1.f;
        else if (std::abs(face - mx[i]) < eps) (&hit.Normal.X)[i] =  1.f;
    }
    return true;
}

} // namespace NF
