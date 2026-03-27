#pragma once
/**
 * @file NavMeshBuilder.h
 * @brief Navigation mesh builder: voxelise geometry, region growing, contour/polygon mesh.
 *
 * Features:
 *   - SetBounds(min, max): world-space AABB for the nav mesh
 *   - SetCellSize(cs) / SetCellHeight(ch): voxel grid resolution
 *   - SetAgentParams(radius, height, maxClimb, maxSlopeDeg): walkability filters
 *   - AddTriangleSoup(verts, vertCount, tris, triCount, areaId): feed geometry
 *   - Build() → bool: run full pipeline (voxel→region→contour→polygon)
 *   - GetPolyCount() → uint32_t
 *   - GetPolyVerts(polyIdx, outVerts[], maxVerts) → uint32_t
 *   - GetPolyNeighbours(polyIdx, outNeighbours[]) → uint32_t: adjacent polygon indices
 *   - FindPath(startPos, endPos, outPath[], maxPoints) → uint32_t: simple graph A*
 *   - SetOnBuildComplete(cb): async-ready callback
 *   - GetPolyCenter(polyIdx) → Vec3
 *   - IsWalkable(pos) → bool
 *   - Clear(): discard current mesh
 *   - Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct NMBVec3 { float x, y, z; };

class NavMeshBuilder {
public:
    NavMeshBuilder();
    ~NavMeshBuilder();

    void Init    ();
    void Shutdown();
    void Clear   ();

    // Geometry input
    void SetBounds   (NMBVec3 minBB, NMBVec3 maxBB);
    void SetCellSize (float cs);
    void SetCellHeight(float ch);
    void SetAgentParams(float radius, float height,
                        float maxClimb, float maxSlopeDeg);
    void AddTriangleSoup(const float* verts, uint32_t vertCount,
                         const uint32_t* tris, uint32_t triCount,
                         uint8_t areaId = 1);

    // Build
    bool Build();
    void SetOnBuildComplete(std::function<void(bool)> cb);

    // Query
    uint32_t GetPolyCount  () const;
    uint32_t GetPolyVerts  (uint32_t polyIdx, NMBVec3* outVerts, uint32_t maxVerts) const;
    uint32_t GetPolyNeighbours(uint32_t polyIdx, uint32_t* outNeighbours) const;
    NMBVec3  GetPolyCenter (uint32_t polyIdx) const;
    bool     IsWalkable    (NMBVec3 pos) const;

    // Pathfinding
    uint32_t FindPath(NMBVec3 start, NMBVec3 end,
                      NMBVec3* outPath, uint32_t maxPoints) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
