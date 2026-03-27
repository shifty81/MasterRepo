#pragma once
/**
 * @file RopeRenderer.h
 * @brief Catenary/Verlet rope mesh generation for rendering: segments, sag, twist, caps.
 *
 * Features:
 *   - CreateRope(id, segmentCount, thickness): register a renderable rope
 *   - DestroyRope(id)
 *   - SetEndpoints(id, startPos, endPos): update attachment points
 *   - SetSag(id, sagFactor): 0=taut … 1=fully sagging catenary
 *   - SetTwist(id, twistAngle): rotational twist along rope length
 *   - SetSegmentCount(id, n): change tessellation
 *   - SetThickness(id, r): cross-section radius
 *   - SetUVTile(id, uTile): UV tiling along length
 *   - GetVertices(id, outPos[], outNorm[], outUV[], outCount): computed mesh data
 *   - GetIndices(id, outIdx[], outCount)
 *   - EnableCaps(id, on): generate hemisphere end-caps
 *   - UpdateSimulation(id, dt, gravity): advance Verlet integration from given endpoints
 *   - FixEndpoint(id, end, fixed): pin start(0) or end(1) to its position
 *   - GetMidpointPos(id) → Vec3: position of middle segment node
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <vector>

namespace Engine {

struct RRVec2 { float u,v; };
struct RRVec3 { float x,y,z; };

class RopeRenderer {
public:
    RopeRenderer();
    ~RopeRenderer();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Rope lifecycle
    void CreateRope (uint32_t id, uint32_t segmentCount=16, float thickness=0.05f);
    void DestroyRope(uint32_t id);

    // Shape control
    void SetEndpoints   (uint32_t id, RRVec3 start, RRVec3 end);
    void SetSag         (uint32_t id, float sagFactor);
    void SetTwist       (uint32_t id, float twistAngle);
    void SetSegmentCount(uint32_t id, uint32_t n);
    void SetThickness   (uint32_t id, float r);
    void SetUVTile      (uint32_t id, float uTile);
    void EnableCaps     (uint32_t id, bool on);

    // Simulation
    void UpdateSimulation(uint32_t id, float dt, RRVec3 gravity={0,-9.81f,0});
    void FixEndpoint     (uint32_t id, uint32_t endIndex, bool fixed);

    // Mesh output
    void GetVertices(uint32_t id, std::vector<RRVec3>& outPos,
                     std::vector<RRVec3>& outNorm,
                     std::vector<RRVec2>& outUV) const;
    void GetIndices (uint32_t id, std::vector<uint32_t>& outIdx) const;

    // Query
    RRVec3 GetMidpointPos(uint32_t id) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
