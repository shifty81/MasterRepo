#pragma once
/**
 * @file ProceduralMeshSystem.h
 * @brief Runtime mesh generation: primitives, extrude, lathe, boolean ops, normals.
 *
 * Features:
 *   - CreateMesh(id) / DestroyMesh(id)
 *   - AddVertex(meshId, x, y, z, nx, ny, nz, u, v) → uint32_t vertexIndex
 *   - AddTriangle(meshId, a, b, c)
 *   - BuildBox(meshId, w, h, d)
 *   - BuildSphere(meshId, radius, rings, slices)
 *   - BuildCylinder(meshId, radius, height, slices)
 *   - BuildPlane(meshId, w, h, subdX, subdY)
 *   - BuildTorus(meshId, majorR, minorR, majSeg, minSeg)
 *   - Extrude(meshId, profileX[], profileY[], count, depth)
 *   - RecalcNormals(meshId): flat or smooth
 *   - GetVertexCount(meshId) / GetTriangleCount(meshId) → uint32_t
 *   - GetVertexPos(meshId, idx, outX, outY, outZ)
 *   - GetVertexUV(meshId, idx, outU, outV)
 *   - Clear(meshId): remove all geometry
 *   - MergeMesh(dstId, srcId): append src into dst
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <vector>

namespace Engine {

class ProceduralMeshSystem {
public:
    ProceduralMeshSystem();
    ~ProceduralMeshSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Mesh management
    void CreateMesh (uint32_t id);
    void DestroyMesh(uint32_t id);
    void Clear      (uint32_t id);
    void MergeMesh  (uint32_t dstId, uint32_t srcId);

    // Geometry building
    uint32_t AddVertex  (uint32_t id, float x, float y, float z,
                         float nx = 0, float ny = 1, float nz = 0,
                         float u = 0, float v = 0);
    void     AddTriangle(uint32_t id, uint32_t a, uint32_t b, uint32_t c);

    // Primitives
    void BuildBox     (uint32_t id, float w, float h, float d);
    void BuildSphere  (uint32_t id, float radius,
                       uint32_t rings = 16, uint32_t slices = 16);
    void BuildCylinder(uint32_t id, float radius, float height,
                       uint32_t slices = 16);
    void BuildPlane   (uint32_t id, float w, float h,
                       uint32_t subdX = 1, uint32_t subdY = 1);
    void BuildTorus   (uint32_t id, float majorR, float minorR,
                       uint32_t majSeg = 24, uint32_t minSeg = 12);

    // Operations
    void Extrude      (uint32_t id, const std::vector<float>& profileX,
                       const std::vector<float>& profileY, float depth);
    void RecalcNormals(uint32_t id, bool smooth = true);

    // Query
    uint32_t GetVertexCount  (uint32_t id) const;
    uint32_t GetTriangleCount(uint32_t id) const;
    void     GetVertexPos    (uint32_t id, uint32_t idx,
                              float& x, float& y, float& z) const;
    void     GetVertexUV     (uint32_t id, uint32_t idx,
                              float& u, float& v) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
