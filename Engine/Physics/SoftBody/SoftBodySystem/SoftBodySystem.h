#pragma once
/**
 * @file SoftBodySystem.h
 * @brief Mass-spring soft body: vertices, springs, gravity, pressure, sphere collision.
 *
 * Features:
 *   - SoftBodyDesc: vertex count, rest positions, mass, spring stiffness/damping
 *   - Spring types: stretch, shear, bend
 *   - Gravity and external forces
 *   - Volume preservation: pressure force (pneumatic model)
 *   - Sphere and plane collision response
 *   - Pin constraints: lock vertices to world positions
 *   - Verlet integration with velocity damping
 *   - Tick: integrate → solve springs → collision → apply pins
 *   - GetVertex(id, idx) → pos
 *   - AddImpulse per vertex
 *   - Multiple soft bodies
 *   - On-collision callback
 *
 * Typical usage:
 * @code
 *   SoftBodySystem sb;
 *   sb.Init();
 *   SoftBodyDesc d; d.vertexCount=100; // fill d.positions...
 *   d.pressure=0.3f; d.stiffness=0.8f;
 *   uint32_t id = sb.Create(d, {0,5,0});
 *   sb.Pin(id, 0, {0,5,0});
 *   sb.Tick(dt);
 *   const float* p = sb.GetVertexPos(id, 50);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct SoftBodyDesc {
    uint32_t vertexCount{0};
    // vertex positions provided via SetRestPositions()
    float    mass       {0.05f};    ///< kg per vertex
    float    stiffness  {0.8f};     ///< spring stiffness [0,1]
    float    damping    {0.02f};
    float    pressure   {0.2f};     ///< pneumatic pressure coefficient
    float    gravity[3] {0,-9.81f,0};
    bool     selfCollision{false};
};

struct SBSphereCollider { float centre[3]{}; float radius{0.5f}; };
struct SBPlaneCollider  { float normal[3]{0,1,0}; float d{0.f}; };

class SoftBodySystem {
public:
    SoftBodySystem();
    ~SoftBodySystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    // Lifecycle
    uint32_t Create (const SoftBodyDesc& desc, const float origin[3]=nullptr);
    void     Destroy(uint32_t id);
    bool     Has    (uint32_t id) const;

    // Setup
    void SetRestPositions(uint32_t id, const float* posArray, uint32_t count);
    void AddSpring(uint32_t id, uint32_t a, uint32_t b, float restLen=-1.f);
    void AutoSprings(uint32_t id);   ///< build springs from convex hull

    // Pin / forces
    void Pin  (uint32_t id, uint32_t vi, const float worldPos[3]);
    void Unpin(uint32_t id, uint32_t vi);
    void AddImpulse(uint32_t id, uint32_t vi, const float impulse[3]);

    // Colliders
    void AddSphere(uint32_t id, const SBSphereCollider& s);
    void AddPlane (uint32_t id, const SBPlaneCollider&  p);
    void ClearColliders(uint32_t id);

    // Access
    const float* GetVertexPos(uint32_t id, uint32_t vi) const;
    uint32_t     VertexCount (uint32_t id) const;

    // Callbacks
    void SetOnCollision(std::function<void(uint32_t id, uint32_t vi)> cb);

    std::vector<uint32_t> GetAll() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
