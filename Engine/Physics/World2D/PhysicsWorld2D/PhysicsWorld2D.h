#pragma once
/**
 * @file PhysicsWorld2D.h
 * @brief 2-D rigid-body physics: AABB/circle bodies, integrate, collide, resolve.
 *
 * Features:
 *   - BodyType: Static, Kinematic, Dynamic
 *   - ShapeType: AABB, Circle
 *   - AddBody(id, type, shapeType, x, y, hw, hh, mass) → bool
 *   - RemoveBody(id)
 *   - SetVelocity(id, vx, vy) / GetVelocity(id, vx, vy)
 *   - ApplyForce(id, fx, fy) / ApplyImpulse(id, ix, iy)
 *   - SetPosition(id, x, y) / GetPosition(id, x, y)
 *   - SetGravity(gx, gy) / SetDamping(linear)
 *   - Tick(dt): integrate + broadphase + narrowphase + resolution
 *   - GetContactPairs(out[]) → uint32_t: (bodyA, bodyB) pairs this frame
 *   - Raycast(ox, oy, dx, dy, maxDist, outId, outX, outY, outNx, outNy) → bool
 *   - SetOnCollide(cb): callback(idA, idB) on contact
 *   - SetRestitution(id, e) / SetFriction(id, f)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>
#include <utility>

namespace Engine {

enum class BodyType2D  : uint8_t { Static, Kinematic, Dynamic };
enum class ShapeType2D : uint8_t { AABB, Circle };

class PhysicsWorld2D {
public:
    PhysicsWorld2D();
    ~PhysicsWorld2D();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Bodies
    bool AddBody   (uint32_t id, BodyType2D type, ShapeType2D shape,
                    float x, float y, float hw, float hh, float mass = 1.f);
    void RemoveBody(uint32_t id);

    // Motion
    void SetVelocity(uint32_t id, float vx, float vy);
    void GetVelocity(uint32_t id, float& vx, float& vy) const;
    void ApplyForce  (uint32_t id, float fx, float fy);
    void ApplyImpulse(uint32_t id, float ix, float iy);

    // Position
    void SetPosition(uint32_t id, float x, float y);
    void GetPosition(uint32_t id, float& x, float& y) const;

    // Material
    void SetRestitution(uint32_t id, float e);
    void SetFriction   (uint32_t id, float f);

    // World
    void SetGravity(float gx, float gy);
    void SetDamping(float linear);

    // Per-frame
    void Tick(float dt);

    // Query
    uint32_t GetContactPairs(std::vector<std::pair<uint32_t,uint32_t>>& out) const;
    bool     Raycast(float ox, float oy, float dx, float dy, float maxDist,
                     uint32_t& outId, float& outX, float& outY,
                     float& outNx, float& outNy) const;

    // Callback
    void SetOnCollide(std::function<void(uint32_t idA, uint32_t idB)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
