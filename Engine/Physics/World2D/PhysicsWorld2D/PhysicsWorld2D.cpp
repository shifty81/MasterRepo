#include "Engine/Physics/World2D/PhysicsWorld2D/PhysicsWorld2D.h"
#include <cmath>
#include <unordered_map>

namespace Engine {

struct Body2D {
    BodyType2D  type;
    ShapeType2D shape;
    float x{0}, y{0};
    float hw{1}, hh{1};
    float vx{0}, vy{0};
    float mass{1}, invMass{1};
    float restitution{0.3f}, friction{0.4f};
    float fx{0}, fy{0};
    bool  active{true};
};

struct PhysicsWorld2D::Impl {
    std::unordered_map<uint32_t,Body2D> bodies;
    std::vector<std::pair<uint32_t,uint32_t>> contacts;
    float gx{0}, gy{-9.81f};
    float damping{0.99f};
    std::function<void(uint32_t,uint32_t)> onCollide;
};

PhysicsWorld2D::PhysicsWorld2D()  { m_impl = new Impl; }
PhysicsWorld2D::~PhysicsWorld2D() { delete m_impl; }

void PhysicsWorld2D::Init    () {}
void PhysicsWorld2D::Shutdown() { Reset(); }
void PhysicsWorld2D::Reset   () { m_impl->bodies.clear(); m_impl->contacts.clear(); }

bool PhysicsWorld2D::AddBody(uint32_t id, BodyType2D type, ShapeType2D shape,
                              float x, float y, float hw, float hh, float mass) {
    Body2D b;
    b.type = type; b.shape = shape;
    b.x = x; b.y = y; b.hw = hw; b.hh = hh;
    b.mass = (mass > 0 && type == BodyType2D::Dynamic) ? mass : 0;
    b.invMass = b.mass > 0 ? 1.f/b.mass : 0.f;
    m_impl->bodies[id] = b;
    return true;
}

void PhysicsWorld2D::RemoveBody(uint32_t id) { m_impl->bodies.erase(id); }

void PhysicsWorld2D::SetVelocity(uint32_t id, float vx, float vy) {
    auto it = m_impl->bodies.find(id);
    if (it != m_impl->bodies.end()) { it->second.vx = vx; it->second.vy = vy; }
}
void PhysicsWorld2D::GetVelocity(uint32_t id, float& vx, float& vy) const {
    auto it = m_impl->bodies.find(id);
    vx = it != m_impl->bodies.end() ? it->second.vx : 0;
    vy = it != m_impl->bodies.end() ? it->second.vy : 0;
}
void PhysicsWorld2D::ApplyForce  (uint32_t id, float fx, float fy) {
    auto it = m_impl->bodies.find(id);
    if (it != m_impl->bodies.end()) { it->second.fx += fx; it->second.fy += fy; }
}
void PhysicsWorld2D::ApplyImpulse(uint32_t id, float ix, float iy) {
    auto it = m_impl->bodies.find(id);
    if (it != m_impl->bodies.end() && it->second.invMass > 0) {
        it->second.vx += ix * it->second.invMass;
        it->second.vy += iy * it->second.invMass;
    }
}
void PhysicsWorld2D::SetPosition(uint32_t id, float x, float y) {
    auto it = m_impl->bodies.find(id);
    if (it != m_impl->bodies.end()) { it->second.x = x; it->second.y = y; }
}
void PhysicsWorld2D::GetPosition(uint32_t id, float& x, float& y) const {
    auto it = m_impl->bodies.find(id);
    x = it != m_impl->bodies.end() ? it->second.x : 0;
    y = it != m_impl->bodies.end() ? it->second.y : 0;
}
void PhysicsWorld2D::SetRestitution(uint32_t id, float e) {
    auto it = m_impl->bodies.find(id); if (it != m_impl->bodies.end()) it->second.restitution = e;
}
void PhysicsWorld2D::SetFriction(uint32_t id, float f) {
    auto it = m_impl->bodies.find(id); if (it != m_impl->bodies.end()) it->second.friction = f;
}
void PhysicsWorld2D::SetGravity(float gx, float gy) { m_impl->gx = gx; m_impl->gy = gy; }
void PhysicsWorld2D::SetDamping(float linear)        { m_impl->damping = linear; }

void PhysicsWorld2D::Tick(float dt) {
    // integrate
    for (auto& [id, b] : m_impl->bodies) {
        if (b.type != BodyType2D::Dynamic || !b.active) continue;
        float ax = (b.fx + m_impl->gx) * b.invMass;
        float ay = (b.fy + m_impl->gy) * b.invMass;
        b.vx = (b.vx + ax * dt) * m_impl->damping;
        b.vy = (b.vy + ay * dt) * m_impl->damping;
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        b.fx = b.fy = 0;
    }
    // simple AABB broadphase + narrow
    m_impl->contacts.clear();
    std::vector<std::pair<uint32_t,Body2D*>> list;
    for (auto& [id, b] : m_impl->bodies) if (b.active) list.push_back({id,&b});
    for (size_t i = 0; i < list.size(); ++i)
    for (size_t j = i+1; j < list.size(); ++j) {
        auto& [ia, ba] = list[i];
        auto& [ib, bb] = list[j];
        if (ba->type == BodyType2D::Static && bb->type == BodyType2D::Static) continue;
        float ox = std::abs(ba->x - bb->x) - (ba->hw + bb->hw);
        float oy = std::abs(ba->y - bb->y) - (ba->hh + bb->hh);
        if (ox < 0 && oy < 0) {
            m_impl->contacts.push_back({ia, ib});
            if (m_impl->onCollide) m_impl->onCollide(ia, ib);
            // resolve
            float e = (ba->restitution + bb->restitution) * 0.5f;
            if (ox > oy) {
                float sign = ba->x < bb->x ? -1.f : 1.f;
                float rel = (ba->vx - bb->vx) * sign;
                if (rel > 0) {
                    float j2 = -(1+e)*rel / (ba->invMass + bb->invMass);
                    ba->vx += j2 * ba->invMass * sign;
                    bb->vx -= j2 * bb->invMass * sign;
                }
            } else {
                float sign = ba->y < bb->y ? -1.f : 1.f;
                float rel = (ba->vy - bb->vy) * sign;
                if (rel > 0) {
                    float j2 = -(1+e)*rel / (ba->invMass + bb->invMass);
                    ba->vy += j2 * ba->invMass * sign;
                    bb->vy -= j2 * bb->invMass * sign;
                }
            }
        }
    }
}

uint32_t PhysicsWorld2D::GetContactPairs(std::vector<std::pair<uint32_t,uint32_t>>& out) const {
    out = m_impl->contacts;
    return (uint32_t)out.size();
}

bool PhysicsWorld2D::Raycast(float ox, float oy, float dx, float dy, float maxDist,
                              uint32_t& outId, float& outX, float& outY,
                              float& outNx, float& outNy) const {
    float best = maxDist;
    bool hit = false;
    for (auto& [id, b] : m_impl->bodies) {
        // slab test vs AABB
        float minX = b.x - b.hw, maxX = b.x + b.hw;
        float minY = b.y - b.hh, maxY = b.y + b.hh;
        float tmin = 0, tmax = best;
        if (std::abs(dx) < 1e-6f) { if (ox < minX || ox > maxX) continue; }
        else {
            float t1 = (minX - ox)/dx, t2 = (maxX - ox)/dx;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            tmin = std::max(tmin, t1); tmax = std::min(tmax, t2);
        }
        if (std::abs(dy) < 1e-6f) { if (oy < minY || oy > maxY) continue; }
        else {
            float t1 = (minY - oy)/dy, t2 = (maxY - oy)/dy;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            tmin = std::max(tmin, t1); tmax = std::min(tmax, t2);
        }
        if (tmin < tmax && tmin < best) {
            best = tmin; hit = true; outId = id;
            outX = ox + dx*tmin; outY = oy + dy*tmin;
            outNx = 0; outNy = 0;
        }
    }
    return hit;
}

void PhysicsWorld2D::SetOnCollide(std::function<void(uint32_t,uint32_t)> cb) {
    m_impl->onCollide = std::move(cb);
}

} // namespace Engine
