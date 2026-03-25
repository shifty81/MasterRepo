#include "Engine/Raycast/RaycastSystem/RaycastSystem.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace Engine {

// ── helpers ───────────────────────────────────────────────────────────────────

static float Dot(const Vec3f& a, const Vec3f& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static Vec3f Cross(const Vec3f& a, const Vec3f& b) {
    return { a.y*b.z - a.z*b.y,
             a.z*b.x - a.x*b.z,
             a.x*b.y - a.y*b.x };
}

static float Length(const Vec3f& v) {
    return std::sqrt(Dot(v, v));
}

static Vec3f Normalize(const Vec3f& v) {
    float len = Length(v);
    if (len < 1e-9f) return {0,0,0};
    return v * (1.0f / len);
}

// ── Impl ─────────────────────────────────────────────────────────────────────

struct RaycastSystem::Impl {
    std::vector<Collider>              colliders;
    uint64_t                           nextId{1};
    std::function<void(const RayHit&)> onHit;
    RaycastStats                       stats{};
};

// ── RaycastSystem ─────────────────────────────────────────────────────────────

RaycastSystem::RaycastSystem() : m_impl(new Impl{}) {}
RaycastSystem::~RaycastSystem() { delete m_impl; }

// ── Static intersection tests ─────────────────────────────────────────────────

std::optional<float> RaycastSystem::IntersectAABB(const Ray& ray, const AABB& aabb) {
    float tmin = -std::numeric_limits<float>::infinity();
    float tmax =  std::numeric_limits<float>::infinity();

    const float* origin = &ray.origin.x;
    const float* dir    = &ray.direction.x;
    const float* bmin   = &aabb.min.x;
    const float* bmax   = &aabb.max.x;

    for (int i = 0; i < 3; ++i) {
        if (std::abs(dir[i]) < 1e-9f) {
            if (origin[i] < bmin[i] || origin[i] > bmax[i]) return std::nullopt;
        } else {
            float t1 = (bmin[i] - origin[i]) / dir[i];
            float t2 = (bmax[i] - origin[i]) / dir[i];
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            if (tmin > tmax) return std::nullopt;
        }
    }
    if (tmax < 0.0f) return std::nullopt;
    return tmin < 0.0f ? tmax : tmin;
}

std::optional<float> RaycastSystem::IntersectSphere(const Ray& ray, const Sphere& sphere) {
    Vec3f oc = ray.origin - sphere.center;
    float a  = Dot(ray.direction, ray.direction);
    float b  = 2.0f * Dot(oc, ray.direction);
    float c  = Dot(oc, oc) - sphere.radius * sphere.radius;
    float disc = b*b - 4.0f*a*c;
    if (disc < 0.0f) return std::nullopt;
    float sq = std::sqrt(disc);
    float t1 = (-b - sq) / (2.0f * a);
    float t2 = (-b + sq) / (2.0f * a);
    if (t1 >= 0.0f) return t1;
    if (t2 >= 0.0f) return t2;
    return std::nullopt;
}

std::optional<float> RaycastSystem::IntersectPlane(const Ray& ray, const Plane& plane) {
    float denom = Dot(plane.normal, ray.direction);
    if (std::abs(denom) < 1e-9f) return std::nullopt;
    float t = -(Dot(plane.normal, ray.origin) + plane.d) / denom;
    if (t < 0.0f) return std::nullopt;
    return t;
}

std::optional<float> RaycastSystem::IntersectTriangle(const Ray& ray,
                                                       const Triangle& tri) {
    // Möller–Trumbore
    Vec3f e1 = tri.v1 - tri.v0;
    Vec3f e2 = tri.v2 - tri.v0;
    Vec3f h  = Cross(ray.direction, e2);
    float a  = Dot(e1, h);
    if (std::abs(a) < 1e-9f) return std::nullopt;
    float f  = 1.0f / a;
    Vec3f s  = ray.origin - tri.v0;
    float u  = f * Dot(s, h);
    if (u < 0.0f || u > 1.0f) return std::nullopt;
    Vec3f q  = Cross(s, e1);
    float v  = f * Dot(ray.direction, q);
    if (v < 0.0f || u + v > 1.0f) return std::nullopt;
    float t  = f * Dot(e2, q);
    if (t < 1e-9f) return std::nullopt;
    return t;
}

Vec3f RaycastSystem::PointAt(const Ray& ray, float t) {
    return ray.origin + ray.direction * t;
}

// ── Registry ──────────────────────────────────────────────────────────────────

static uint64_t AddColliderToVec(std::vector<Collider>& v, uint64_t& nextId, Collider c) {
    c.id = nextId++;
    v.push_back(c);
    return c.id;
}

uint64_t RaycastSystem::AddAABB(const AABB& aabb, const std::string& tag) {
    Collider c; c.type = ColliderType::AABB; c.aabb = aabb; c.tag = tag;
    return AddColliderToVec(m_impl->colliders, m_impl->nextId, c);
}
uint64_t RaycastSystem::AddSphere(const Sphere& sphere, const std::string& tag) {
    Collider c; c.type = ColliderType::Sphere; c.sphere = sphere; c.tag = tag;
    return AddColliderToVec(m_impl->colliders, m_impl->nextId, c);
}
uint64_t RaycastSystem::AddPlane(const Plane& plane, const std::string& tag) {
    Collider c; c.type = ColliderType::Plane; c.plane = plane; c.tag = tag;
    return AddColliderToVec(m_impl->colliders, m_impl->nextId, c);
}
uint64_t RaycastSystem::AddTriangle(const Triangle& tri, const std::string& tag) {
    Collider c; c.type = ColliderType::Triangle; c.tri = tri; c.tag = tag;
    return AddColliderToVec(m_impl->colliders, m_impl->nextId, c);
}

void RaycastSystem::Remove(uint64_t id) {
    auto& v = m_impl->colliders;
    v.erase(std::remove_if(v.begin(), v.end(),
                           [id](const Collider& c){ return c.id == id; }),
            v.end());
}

void RaycastSystem::Clear() { m_impl->colliders.clear(); }

size_t RaycastSystem::ColliderCount() const { return m_impl->colliders.size(); }

// ── Scene queries ─────────────────────────────────────────────────────────────

static RayHit TestCollider(const Ray& ray, const Collider& col) {
    std::optional<float> t;
    Vec3f normal{0,0,1};

    switch (col.type) {
    case ColliderType::AABB: {
        t = RaycastSystem::IntersectAABB(ray, col.aabb);
        if (t) {
            // Compute face normal from hit point
            Vec3f p   = RaycastSystem::PointAt(ray, *t);
            Vec3f c   = (col.aabb.min + col.aabb.max) * 0.5f;
            Vec3f d   = (col.aabb.max - col.aabb.min) * 0.5f;
            Vec3f diff = p - c;
            float ax = std::abs(diff.x / (d.x + 1e-9f));
            float ay = std::abs(diff.y / (d.y + 1e-9f));
            float az = std::abs(diff.z / (d.z + 1e-9f));
            if (ax > ay && ax > az) normal = { diff.x > 0 ? 1.f : -1.f, 0, 0 };
            else if (ay > az)       normal = { 0, diff.y > 0 ? 1.f : -1.f, 0 };
            else                    normal = { 0, 0, diff.z > 0 ? 1.f : -1.f };
        }
        break;
    }
    case ColliderType::Sphere: {
        t = RaycastSystem::IntersectSphere(ray, col.sphere);
        if (t) {
            Vec3f p = RaycastSystem::PointAt(ray, *t);
            normal  = p - col.sphere.center;
            float len = std::sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
            if (len > 1e-9f) normal = normal * (1.0f / len);
        }
        break;
    }
    case ColliderType::Plane:
        t = RaycastSystem::IntersectPlane(ray, col.plane);
        if (t) normal = col.plane.normal;
        break;
    case ColliderType::Triangle: {
        t = RaycastSystem::IntersectTriangle(ray, col.tri);
        if (t) {
            Vec3f e1 = col.tri.v1 - col.tri.v0;
            Vec3f e2 = col.tri.v2 - col.tri.v0;
            normal.x = e1.y*e2.z - e1.z*e2.y;
            normal.y = e1.z*e2.x - e1.x*e2.z;
            normal.z = e1.x*e2.y - e1.y*e2.x;
            float len = std::sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
            if (len > 1e-9f) normal = normal * (1.0f / len);
        }
        break;
    }
    }

    if (!t) return RayHit{};
    RayHit hit;
    hit.hit      = true;
    hit.tMin     = *t;
    hit.position = RaycastSystem::PointAt(ray, *t);
    hit.normal   = normal;
    hit.objectId = col.id;
    hit.tag      = col.tag;
    return hit;
}

RayHit RaycastSystem::CastRay(const Ray& ray) const {
    ++m_impl->stats.totalQueries;
    RayHit best{};
    best.tMin = std::numeric_limits<float>::infinity();

    for (const auto& col : m_impl->colliders) {
        RayHit h = TestCollider(ray, col);
        if (h.hit && h.tMin < best.tMin) best = h;
    }

    if (best.hit) {
        ++m_impl->stats.hitCount;
        if (m_impl->onHit) m_impl->onHit(best);
    } else {
        ++m_impl->stats.missCount;
    }
    return best;
}

std::vector<RayHit> RaycastSystem::CastRayAll(const Ray& ray) const {
    ++m_impl->stats.totalQueries;
    std::vector<RayHit> hits;
    for (const auto& col : m_impl->colliders) {
        RayHit h = TestCollider(ray, col);
        if (h.hit) hits.push_back(h);
    }
    std::sort(hits.begin(), hits.end(),
              [](const RayHit& a, const RayHit& b){ return a.tMin < b.tMin; });
    m_impl->stats.hitCount  += static_cast<uint64_t>(hits.size());
    m_impl->stats.missCount += hits.empty() ? 1 : 0;
    return hits;
}

RayHit RaycastSystem::CastRayTag(const Ray& ray, const std::string& tag) const {
    ++m_impl->stats.totalQueries;
    RayHit best{};
    best.tMin = std::numeric_limits<float>::infinity();

    for (const auto& col : m_impl->colliders) {
        if (col.tag.find(tag) == std::string::npos) continue;
        RayHit h = TestCollider(ray, col);
        if (h.hit && h.tMin < best.tMin) best = h;
    }

    if (best.hit) ++m_impl->stats.hitCount;
    else          ++m_impl->stats.missCount;
    return best;
}

RayHit RaycastSystem::ScreenRay(float ndcX, float ndcY,
                                 const float viewProjInv[16]) const {
    // Unproject near and far points
    auto transform = [&](float x, float y, float z) -> Vec3f {
        float w = viewProjInv[3]*x + viewProjInv[7]*y + viewProjInv[11]*z + viewProjInv[15];
        Vec3f p;
        p.x = (viewProjInv[0]*x + viewProjInv[4]*y + viewProjInv[ 8]*z + viewProjInv[12]) / w;
        p.y = (viewProjInv[1]*x + viewProjInv[5]*y + viewProjInv[ 9]*z + viewProjInv[13]) / w;
        p.z = (viewProjInv[2]*x + viewProjInv[6]*y + viewProjInv[10]*z + viewProjInv[14]) / w;
        return p;
    };

    Vec3f near = transform(ndcX, ndcY, -1.0f);
    Vec3f far  = transform(ndcX, ndcY,  1.0f);
    Vec3f dir  = far - near;
    float len  = std::sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    if (len < 1e-9f) return RayHit{};
    dir = dir * (1.0f / len);

    Ray ray{ near, dir };
    return CastRay(ray);
}

void RaycastSystem::OnHit(std::function<void(const RayHit&)> callback) {
    m_impl->onHit = std::move(callback);
}

RaycastStats RaycastSystem::Stats() const { return m_impl->stats; }

} // namespace Engine
