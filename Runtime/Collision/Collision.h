#pragma once
#include <cstdint>
#include <vector>

namespace Runtime {

enum class ShapeType { AABB, Sphere, Capsule, ConvexHull };

struct CollisionShape {
    ShapeType type           = ShapeType::AABB;
    float     halfExtents[3] = {0.5f, 0.5f, 0.5f};
    float     radius         = 0.5f;
    float     height         = 2.0f;
};

struct CollisionBody {
    uint32_t       id        = 0;
    uint32_t       entityId  = 0;
    CollisionShape shape;
    float          position[3] = {0.f, 0.f, 0.f};
    float          rotation[4] = {0.f, 0.f, 0.f, 1.f};
    bool           isTrigger = false;
    uint32_t       layer     = 1;
    uint32_t       mask      = 0xFFFFFFFF;
};

struct CollisionResult {
    bool     hit           = false;
    uint32_t bodyA_id      = 0;
    uint32_t bodyB_id      = 0;
    float    contactPoint[3] = {};
    float    normal[3]       = {};
    float    depth           = 0.f;
};

class CollisionWorld {
public:
    uint32_t      AddBody(CollisionBody body);
    void          RemoveBody(uint32_t id);
    CollisionBody*       GetBody(uint32_t id);
    const CollisionBody* GetBody(uint32_t id) const;
    void          UpdateBody(uint32_t id, const float pos[3], const float rot[4]);

    void Step();
    const std::vector<CollisionResult>& QueryResults() const;
    void ClearResults();

    CollisionResult       RayCast(const float origin[3], const float dir[3],
                                   float maxDist, uint32_t layerMask) const;
    std::vector<uint32_t> SphereOverlap(const float center[3], float radius,
                                         uint32_t layerMask) const;

    size_t BodyCount() const;

private:
    bool BroadPhaseOverlap(const CollisionBody& a, const CollisionBody& b) const;

    CollisionResult NarrowPhaseAABB_AABB(const CollisionBody& a, const CollisionBody& b) const;
    CollisionResult NarrowPhaseSphere_Sphere(const CollisionBody& a, const CollisionBody& b) const;
    CollisionResult NarrowPhaseAABB_Sphere(const CollisionBody& aabb, const CollisionBody& sphere) const;

    uint32_t m_nextId = 1;
    std::vector<CollisionBody>   m_bodies;
    std::vector<CollisionResult> m_results;
};

} // namespace Runtime
