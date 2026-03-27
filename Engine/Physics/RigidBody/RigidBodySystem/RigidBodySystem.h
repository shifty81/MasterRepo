#pragma once
/**
 * @file RigidBodySystem.h
 * @brief Discrete rigid body simulation: AABB/sphere shapes, forces, impulses, collision response.
 *
 * Features:
 *   - Body types: Dynamic, Kinematic, Static
 *   - Collision shapes: AABB, Sphere (shared half-extent or radius)
 *   - Per-body properties: mass, friction, restitution, linear/angular damping
 *   - Force accumulation: AddForce, AddImpulse, AddTorque
 *   - Gravity (global + per-body override)
 *   - Discrete collision detection: broad-phase AABB overlap + narrow-phase shape test
 *   - Collision response: penalty impulse with restitution and friction
 *   - Collision callbacks: OnCollisionEnter/Stay/Exit per body pair
 *   - Sub-stepping: configurable substeps per Tick for stability
 *   - Body sleep: auto-sleep when velocity below threshold
 *   - Layer/mask filtering (32-bit bitmask)
 *
 * Typical usage:
 * @code
 *   RigidBodySystem rbs;
 *   rbs.Init();
 *   rbs.SetGravity({0,-9.81f,0});
 *   uint32_t boxId = rbs.CreateBody({BodyType::Dynamic, ShapeType::AABB, 1.f});
 *   rbs.SetPosition(boxId, {0,5,0});
 *   rbs.OnCollisionEnter(boxId, [](uint32_t other){ HitSound(); });
 *   rbs.Tick(dt);
 *   auto pos = rbs.GetPosition(boxId);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class BodyType  : uint8_t { Static=0, Kinematic, Dynamic };
enum class ShapeType : uint8_t { AABB=0, Sphere };

struct BodyDesc {
    BodyType  type    {BodyType::Dynamic};
    ShapeType shape   {ShapeType::AABB};
    float     mass    {1.f};
    float     halfExtent[3]{0.5f,0.5f,0.5f}; ///< for AABB; x=radius for Sphere
    float     friction    {0.5f};
    float     restitution {0.3f};
    float     linearDamp  {0.01f};
    float     angularDamp {0.05f};
    uint32_t  layer   {0x01};
    uint32_t  mask    {0xFF};
};

struct ContactPoint {
    float normal[3]{};
    float point [3]{};
    float depth {0.f};
};

class RigidBodySystem {
public:
    RigidBodySystem();
    ~RigidBodySystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    // Simulation config
    void SetGravity   (const float g[3]);
    void SetSubsteps  (uint32_t n);
    void SetSleepThreshold(float vel);

    // Body management
    uint32_t CreateBody (const BodyDesc& desc);
    void     DestroyBody(uint32_t id);
    bool     HasBody    (uint32_t id) const;

    // Transforms
    void  SetPosition   (uint32_t id, const float pos[3]);
    void  SetOrientation(uint32_t id, const float quat[4]);
    void  GetPosition   (uint32_t id, float out[3]) const;
    void  GetOrientation(uint32_t id, float out[4]) const;

    // Velocity
    void  SetLinearVelocity (uint32_t id, const float v[3]);
    void  SetAngularVelocity(uint32_t id, const float v[3]);
    void  GetLinearVelocity (uint32_t id, float out[3]) const;
    void  GetAngularVelocity(uint32_t id, float out[3]) const;

    // Forces
    void AddForce  (uint32_t id, const float f[3]);
    void AddImpulse(uint32_t id, const float j[3]);
    void AddTorque (uint32_t id, const float t[3]);
    void ClearForces(uint32_t id);

    // Properties
    void  SetMass       (uint32_t id, float mass);
    void  SetFriction   (uint32_t id, float f);
    void  SetRestitution(uint32_t id, float r);
    void  SetKinematic  (uint32_t id, bool b);
    void  SetEnabled    (uint32_t id, bool b);
    float GetMass       (uint32_t id) const;

    // Collision callbacks
    void OnCollisionEnter(uint32_t id, std::function<void(uint32_t other, const ContactPoint&)> cb);
    void OnCollisionStay (uint32_t id, std::function<void(uint32_t other, const ContactPoint&)> cb);
    void OnCollisionExit (uint32_t id, std::function<void(uint32_t other)> cb);

    // Query
    bool Overlap(uint32_t idA, uint32_t idB, ContactPoint* outContact=nullptr) const;
    uint32_t BodyCount() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
