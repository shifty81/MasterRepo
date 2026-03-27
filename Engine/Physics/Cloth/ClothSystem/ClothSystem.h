#pragma once
/**
 * @file ClothSystem.h
 * @brief Position-based cloth simulation: vertices, edges, stretch/shear/bend constraints.
 *
 * Features:
 *   - ClothMesh: vertex positions/masses, edges (stretch), bend pairs
 *   - Per-constraint stiffness and iteration count (Gauss-Seidel solver)
 *   - Gravity and directional wind forces
 *   - Pin constraints: vertices locked to world positions
 *   - Collision: sphere and plane proxies
 *   - SelfCollision toggle
 *   - Per-vertex damping
 *   - Tick: verlet integration → constraint projection → collision response
 *   - Multiple cloth objects; each assigned a unique ID
 *   - GetVertex / SetPin / AddImpulse per vertex
 *   - On-tear callback (when stretch exceeds tearThreshold)
 *
 * Typical usage:
 * @code
 *   ClothSystem cs;
 *   cs.Init();
 *   ClothDesc d; d.rows=16; d.cols=16; d.spacing=0.1f;
 *   uint32_t id = cs.CreateCloth(d, {0,4,0});
 *   cs.Pin(id, 0, {0,4,0}); cs.Pin(id, 15, {1.5f,4,0});
 *   cs.SetWind(id, {1,0,0.5f});
 *   cs.Tick(dt);
 *   const float* pos = cs.GetVertexPos(id, 42);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct ClothDesc {
    uint32_t rows        {16};
    uint32_t cols        {16};
    float    spacing     {0.1f};    ///< metres between adjacent vertices
    float    mass        {0.1f};    ///< kg per vertex
    float    stretchStiff{0.9f};    ///< [0,1]
    float    shearStiff  {0.7f};
    float    bendStiff   {0.2f};
    float    damping     {0.02f};   ///< velocity damping per tick
    float    tearThreshold{2.f};    ///< stretch ratio that triggers tear
    bool     selfCollision{false};
};

struct ClothSphereProxy { float centre[3]{}; float radius{0.3f}; };
struct ClothPlaneProxy  { float normal[3]{0,1,0}; float d{0.f}; };

class ClothSystem {
public:
    ClothSystem();
    ~ClothSystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    // Cloth lifecycle
    uint32_t CreateCloth (const ClothDesc& desc, const float origin[3]=nullptr);
    void     DestroyCloth(uint32_t id);
    bool     HasCloth    (uint32_t id) const;

    // Gravity / wind
    void SetGravity(uint32_t id, const float g[3]);
    void SetWind   (uint32_t id, const float wind[3]);

    // Pin
    void Pin  (uint32_t id, uint32_t vertIdx, const float worldPos[3]);
    void Unpin(uint32_t id, uint32_t vertIdx);

    // Impulse
    void AddImpulse(uint32_t id, uint32_t vertIdx, const float impulse[3]);

    // Collision proxies
    void AddSphere(uint32_t id, const ClothSphereProxy& s);
    void AddPlane (uint32_t id, const ClothPlaneProxy&  p);
    void ClearColliders(uint32_t id);

    // Vertex access
    const float* GetVertexPos(uint32_t id, uint32_t vertIdx) const; ///< ptr to [x,y,z]
    uint32_t     VertexCount (uint32_t id)                   const;

    // Tear callback
    void SetOnTear(std::function<void(uint32_t clothId, uint32_t edgeIdx)> cb);

    std::vector<uint32_t> GetAll() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
