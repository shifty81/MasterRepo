#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace Engine {

// ── Primitive math ─────────────────────────────────────────────────────────--

struct Vec3f { float x{0}, y{0}, z{0}; };

inline Vec3f operator+(const Vec3f& a, const Vec3f& b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
inline Vec3f operator-(const Vec3f& a, const Vec3f& b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
inline Vec3f operator*(const Vec3f& v, float s)        { return {v.x*s, v.y*s, v.z*s}; }

// ── Data types ────────────────────────────────────────────────────────────────

struct Ray {
    Vec3f origin;
    Vec3f direction;   // should be normalised before passing to queries
};

struct AABB {
    Vec3f min;
    Vec3f max;
};

struct Sphere {
    Vec3f   center;
    float   radius{1.0f};
};

struct Plane {
    Vec3f  normal;     // must be unit length
    float  d{0.0f};    // plane equation: dot(normal, p) + d = 0
};

struct Triangle {
    Vec3f v0, v1, v2;
};

struct RayHit {
    bool     hit{false};
    float    tMin{0.0f};   // parameter along ray at first intersection
    Vec3f    position;     // world-space hit point
    Vec3f    normal;       // surface normal at hit point
    uint64_t objectId{0};  // optional tag set by the caller
    std::string tag;
};

struct RaycastStats {
    uint64_t totalQueries{0};
    uint64_t hitCount{0};
    uint64_t missCount{0};
};

// ── Registered collidable ─────────────────────────────────────────────────────

enum class ColliderType : uint8_t { AABB, Sphere, Plane, Triangle };

struct Collider {
    ColliderType type{ColliderType::AABB};
    AABB         aabb;
    Sphere       sphere;
    Plane        plane;
    Triangle     tri;
    uint64_t     id{0};
    std::string  tag;
};

// ── RaycastSystem ─────────────────────────────────────────────────────────────

class RaycastSystem {
public:
    RaycastSystem();
    ~RaycastSystem();

    // ── Static intersection tests (no state needed) ──────────────────────────

    static std::optional<float> IntersectAABB(const Ray& ray, const AABB& aabb);
    static std::optional<float> IntersectSphere(const Ray& ray, const Sphere& sphere);
    static std::optional<float> IntersectPlane(const Ray& ray, const Plane& plane);

    // Möller–Trumbore triangle intersection.
    static std::optional<float> IntersectTriangle(const Ray& ray, const Triangle& tri);

    // Convenience: return world-space point at parameter t along ray.
    static Vec3f PointAt(const Ray& ray, float t);

    // ── Scene collider registry ───────────────────────────────────────────────

    uint64_t AddAABB    (const AABB&     aabb,   const std::string& tag = "");
    uint64_t AddSphere  (const Sphere&   sphere, const std::string& tag = "");
    uint64_t AddPlane   (const Plane&    plane,  const std::string& tag = "");
    uint64_t AddTriangle(const Triangle& tri,    const std::string& tag = "");

    void  Remove(uint64_t id);
    void  Clear();
    size_t ColliderCount() const;

    // ── Scene queries ─────────────────────────────────────────────────────────

    // Returns the closest hit among all registered colliders.
    RayHit CastRay(const Ray& ray) const;

    // Returns ALL colliders hit, sorted nearest-first.
    std::vector<RayHit> CastRayAll(const Ray& ray) const;

    // Returns the first collider hit whose tag matches (substring search).
    RayHit CastRayTag(const Ray& ray, const std::string& tag) const;

    // Hit-test a screen point (NDC) against registered scene geometry.
    // viewProjInv is a flat 4×4 row-major inverse view-projection matrix.
    RayHit ScreenRay(float ndcX, float ndcY,
                     const float viewProjInv[16]) const;

    // Callback fired on every hit.
    void OnHit(std::function<void(const RayHit&)> callback);

    // Stats.
    RaycastStats Stats() const;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
