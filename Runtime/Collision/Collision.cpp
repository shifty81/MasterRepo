#include "Runtime/Collision/Collision.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace Runtime {

uint32_t CollisionWorld::AddBody(CollisionBody body) {
    body.id = m_nextId++;
    m_bodies.push_back(body);
    return body.id;
}

void CollisionWorld::RemoveBody(uint32_t id) {
    m_bodies.erase(std::remove_if(m_bodies.begin(), m_bodies.end(),
        [id](const CollisionBody& b){ return b.id == id; }), m_bodies.end());
}

CollisionBody* CollisionWorld::GetBody(uint32_t id) {
    for (auto& b : m_bodies) if (b.id == id) return &b;
    return nullptr;
}
const CollisionBody* CollisionWorld::GetBody(uint32_t id) const {
    for (const auto& b : m_bodies) if (b.id == id) return &b;
    return nullptr;
}

void CollisionWorld::UpdateBody(uint32_t id, const float pos[3], const float rot[4]) {
    CollisionBody* b = GetBody(id);
    if (!b) return;
    std::memcpy(b->position, pos, sizeof(float)*3);
    std::memcpy(b->rotation, rot, sizeof(float)*4);
}

bool CollisionWorld::BroadPhaseOverlap(const CollisionBody& a, const CollisionBody& b) const {
    if (!(a.mask & b.layer) && !(b.mask & a.layer)) return false;
    float ra = 0.f, rb = 0.f;
    if (a.shape.type == ShapeType::Sphere)       ra = a.shape.radius;
    else if (a.shape.type == ShapeType::Capsule)  ra = a.shape.radius + a.shape.height * 0.5f;
    else ra = std::sqrt(a.shape.halfExtents[0]*a.shape.halfExtents[0] +
                          a.shape.halfExtents[1]*a.shape.halfExtents[1] +
                          a.shape.halfExtents[2]*a.shape.halfExtents[2]);

    if (b.shape.type == ShapeType::Sphere)       rb = b.shape.radius;
    else if (b.shape.type == ShapeType::Capsule)  rb = b.shape.radius + b.shape.height * 0.5f;
    else rb = std::sqrt(b.shape.halfExtents[0]*b.shape.halfExtents[0] +
                          b.shape.halfExtents[1]*b.shape.halfExtents[1] +
                          b.shape.halfExtents[2]*b.shape.halfExtents[2]);

    float dx = a.position[0] - b.position[0];
    float dy = a.position[1] - b.position[1];
    float dz = a.position[2] - b.position[2];
    float distSq = dx*dx + dy*dy + dz*dz;
    float rSum = ra + rb;
    return distSq <= rSum * rSum;
}

CollisionResult CollisionWorld::NarrowPhaseAABB_AABB(const CollisionBody& a, const CollisionBody& b) const {
    CollisionResult r;
    float overlap[3];
    for (int i = 0; i < 3; ++i) {
        float delta = b.position[i] - a.position[i];
        float combined = a.shape.halfExtents[i] + b.shape.halfExtents[i];
        overlap[i] = combined - std::fabs(delta);
        if (overlap[i] <= 0.f) return r;
    }
    r.hit = true; r.bodyA_id = a.id; r.bodyB_id = b.id;
    int axis = (overlap[0] < overlap[1]) ? ((overlap[0] < overlap[2]) ? 0 : 2) :
                                            ((overlap[1] < overlap[2]) ? 1 : 2);
    r.depth = overlap[axis];
    r.normal[axis] = (a.position[axis] < b.position[axis]) ? 1.f : -1.f;
    for (int i = 0; i < 3; ++i)
        r.contactPoint[i] = (a.position[i] + b.position[i]) * 0.5f;
    return r;
}

CollisionResult CollisionWorld::NarrowPhaseSphere_Sphere(const CollisionBody& a, const CollisionBody& b) const {
    CollisionResult r;
    float dx = b.position[0]-a.position[0];
    float dy = b.position[1]-a.position[1];
    float dz = b.position[2]-a.position[2];
    float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    float rSum = a.shape.radius + b.shape.radius;
    if (dist >= rSum) return r;
    r.hit = true; r.bodyA_id = a.id; r.bodyB_id = b.id;
    r.depth = rSum - dist;
    float inv = (dist > 1e-6f) ? 1.f/dist : 1.f;
    r.normal[0] = dx*inv; r.normal[1] = dy*inv; r.normal[2] = dz*inv;
    for (int i = 0; i < 3; ++i)
        r.contactPoint[i] = a.position[i] + r.normal[i] * a.shape.radius;
    return r;
}

CollisionResult CollisionWorld::NarrowPhaseAABB_Sphere(const CollisionBody& aabb, const CollisionBody& sphere) const {
    CollisionResult r;
    float closestPointOnAABB[3];
    for (int i = 0; i < 3; ++i)
        closestPointOnAABB[i] = std::fmax(aabb.position[i] - aabb.shape.halfExtents[i],
                                 std::fmin(sphere.position[i], aabb.position[i] + aabb.shape.halfExtents[i]));
    float dx = sphere.position[0]-closestPointOnAABB[0];
    float dy = sphere.position[1]-closestPointOnAABB[1];
    float dz = sphere.position[2]-closestPointOnAABB[2];
    float distSq = dx*dx + dy*dy + dz*dz;
    if (distSq >= sphere.shape.radius * sphere.shape.radius) return r;
    float dist = std::sqrt(distSq);
    r.hit = true; r.bodyA_id = aabb.id; r.bodyB_id = sphere.id;
    r.depth = sphere.shape.radius - dist;
    float inv = (dist > 1e-6f) ? 1.f/dist : 1.f;
    r.normal[0] = dx*inv; r.normal[1] = dy*inv; r.normal[2] = dz*inv;
    r.contactPoint[0] = closestPointOnAABB[0]; r.contactPoint[1] = closestPointOnAABB[1]; r.contactPoint[2] = closestPointOnAABB[2];
    return r;
}

void CollisionWorld::Step() {
    m_results.clear();
    for (size_t i = 0; i < m_bodies.size(); ++i) {
        for (size_t j = i+1; j < m_bodies.size(); ++j) {
            const auto& a = m_bodies[i];
            const auto& b = m_bodies[j];
            if (!BroadPhaseOverlap(a, b)) continue;

            CollisionResult res;
            auto at = a.shape.type, bt = b.shape.type;
            if      (at == ShapeType::AABB   && bt == ShapeType::AABB)   res = NarrowPhaseAABB_AABB(a, b);
            else if (at == ShapeType::Sphere  && bt == ShapeType::Sphere) res = NarrowPhaseSphere_Sphere(a, b);
            else if (at == ShapeType::AABB    && bt == ShapeType::Sphere) res = NarrowPhaseAABB_Sphere(a, b);
            else if (at == ShapeType::Sphere  && bt == ShapeType::AABB)   res = NarrowPhaseAABB_Sphere(b, a);
            if (res.hit) m_results.push_back(res);
        }
    }
}

const std::vector<CollisionResult>& CollisionWorld::QueryResults() const { return m_results; }
void CollisionWorld::ClearResults() { m_results.clear(); }
size_t CollisionWorld::BodyCount() const { return m_bodies.size(); }

CollisionResult CollisionWorld::RayCast(const float origin[3], const float dir[3],
                                         float maxDist, uint32_t layerMask) const {
    CollisionResult best;
    float bestDist = maxDist;

    for (const auto& body : m_bodies) {
        if (!(body.layer & layerMask)) continue;
        if (body.shape.type == ShapeType::AABB) {
            float tmin = 0.f, tmax = bestDist;
            bool hit = true;
            for (int i = 0; i < 3; ++i) {
                float lo = body.position[i] - body.shape.halfExtents[i];
                float hi = body.position[i] + body.shape.halfExtents[i];
                if (std::fabs(dir[i]) < 1e-8f) {
                    if (origin[i] < lo || origin[i] > hi) { hit = false; break; }
                } else {
                    float t1 = (lo - origin[i]) / dir[i];
                    float t2 = (hi - origin[i]) / dir[i];
                    if (t1 > t2) { float tmp=t1; t1=t2; t2=tmp; }
                    tmin = std::fmax(tmin, t1);
                    tmax = std::fmin(tmax, t2);
                    if (tmin > tmax) { hit = false; break; }
                }
            }
            if (hit && tmin < bestDist) {
                bestDist = tmin;
                best.hit = true; best.bodyA_id = body.id;
                for (int i=0;i<3;++i) best.contactPoint[i] = origin[i] + dir[i]*tmin;
                best.depth = 0.f;
            }
        } else if (body.shape.type == ShapeType::Sphere) {
            float ox = origin[0]-body.position[0];
            float oy = origin[1]-body.position[1];
            float oz = origin[2]-body.position[2];
            float a = dir[0]*dir[0]+dir[1]*dir[1]+dir[2]*dir[2];
            float b2 = 2.f*(ox*dir[0]+oy*dir[1]+oz*dir[2]);
            float c = ox*ox+oy*oy+oz*oz - body.shape.radius*body.shape.radius;
            float disc = b2*b2 - 4.f*a*c;
            if (disc >= 0.f) {
                float t = (-b2 - std::sqrt(disc)) / (2.f*a);
                if (t >= 0.f && t < bestDist) {
                    bestDist = t;
                    best.hit = true; best.bodyA_id = body.id;
                    for (int i=0;i<3;++i) best.contactPoint[i] = origin[i]+dir[i]*t;
                }
            }
        }
    }
    return best;
}

std::vector<uint32_t> CollisionWorld::SphereOverlap(const float center[3], float radius,
                                                      uint32_t layerMask) const {
    std::vector<uint32_t> result;
    for (const auto& body : m_bodies) {
        if (!(body.layer & layerMask)) continue;
        if (body.shape.type == ShapeType::AABB) {
            float closestPointOnAABB[3];
            for (int i = 0; i < 3; ++i)
                closestPointOnAABB[i] = std::fmax(body.position[i]-body.shape.halfExtents[i],
                                         std::fmin(center[i], body.position[i]+body.shape.halfExtents[i]));
            float dx=center[0]-closestPointOnAABB[0], dy=center[1]-closestPointOnAABB[1], dz=center[2]-closestPointOnAABB[2];
            if (dx*dx+dy*dy+dz*dz <= radius*radius) result.push_back(body.id);
        } else if (body.shape.type == ShapeType::Sphere) {
            float dx=center[0]-body.position[0], dy=center[1]-body.position[1], dz=center[2]-body.position[2];
            float rSum = radius + body.shape.radius;
            if (dx*dx+dy*dy+dz*dz <= rSum*rSum) result.push_back(body.id);
        }
    }
    return result;
}

} // namespace Runtime
