#pragma once
/**
 * @file MeshBuilder.h
 * @brief Procedural mesh construction: vertices, triangles, UVs, normals, tangents.
 *
 * Features:
 *   - Fluent builder API: AddVertex, AddTriangle, AddQuad
 *   - Per-vertex: position (vec3), normal (vec3), tangent (vec4), UV0/UV1 (vec2), colour (vec4)
 *   - Indexed and non-indexed mesh modes
 *   - Primitive helpers: AddBox, AddSphere, AddPlane, AddCylinder, AddCapsule, AddCone
 *   - Tangent-space generation from positions and UVs
 *   - Merge two MeshBuilder instances
 *   - Transform: translate, rotate, scale all verts
 *   - Flip normals, flip winding
 *   - Export to flat float arrays (for GPU upload): GetPositions, GetNormals, GetUVs, GetIndices
 *   - Bounding box calculation
 *
 * Typical usage:
 * @code
 *   MeshBuilder mb;
 *   mb.AddBox({0,0,0}, {1,1,1});
 *   mb.RecalcNormals();
 *   mb.RecalcTangents();
 *   auto pos     = mb.GetPositions();  // vector<float> (xyz * N)
 *   auto indices = mb.GetIndices();    // vector<uint32_t>
 *   UploadMesh(pos.data(), indices.data(), ...);
 * @endcode
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Engine {

struct MBVertex {
    float position[3]{};
    float normal  [3]{0,1,0};
    float tangent [4]{1,0,0,1};
    float uv0     [2]{};
    float uv1     [2]{};
    float colour  [4]{1,1,1,1};
};

struct MBBounds {
    float min[3]{};
    float max[3]{};
};

class MeshBuilder {
public:
    MeshBuilder() = default;
    ~MeshBuilder() = default;

    // Clear
    void Clear();

    // Vertex / index management
    uint32_t AddVertex(const MBVertex& v);
    void     AddTriangle(uint32_t a, uint32_t b, uint32_t c);
    uint32_t AddVertex(const float pos[3], const float uv[2]=nullptr,
                        const float normal[3]=nullptr);
    void     AddQuad(uint32_t a, uint32_t b, uint32_t c, uint32_t d); // 2 tris

    // Primitives
    void AddBox     (const float centre[3], const float halfExtents[3]);
    void AddSphere  (const float centre[3], float radius,
                     uint32_t stacks=16, uint32_t slices=16);
    void AddPlane   (const float centre[3], const float normal[3],
                     float halfW, float halfH, uint32_t divsW=1, uint32_t divsH=1);
    void AddCylinder(const float base[3], float radius, float height,
                     uint32_t slices=16, bool caps=true);
    void AddCapsule (const float base[3], float radius, float height,
                     uint32_t stacks=8, uint32_t slices=16);
    void AddCone    (const float base[3], float radius, float height,
                     uint32_t slices=16, bool cap=true);

    // Operations
    void RecalcNormals   ();
    void RecalcTangents  ();
    void FlipNormals     ();
    void FlipWinding     ();
    void Translate       (const float delta[3]);
    void Scale           (const float scale[3]);
    void Rotate          (const float axisAngle[4]); ///< axis xyz, angle w (radians)
    void Merge           (const MeshBuilder& other);

    // Export (flat arrays, ready for GPU)
    std::vector<float>    GetPositions() const;  ///< xyz per vertex
    std::vector<float>    GetNormals  () const;  ///< xyz per vertex
    std::vector<float>    GetTangents () const;  ///< xyzw per vertex
    std::vector<float>    GetUV0      () const;  ///< uv per vertex
    std::vector<float>    GetColours  () const;  ///< rgba per vertex
    std::vector<uint32_t> GetIndices  () const;
    std::vector<MBVertex> GetVertices () const;

    // Stats / query
    uint32_t VertexCount  () const;
    uint32_t TriangleCount() const;
    MBBounds GetBounds    () const;
    bool     IsEmpty      () const;

private:
    std::vector<MBVertex>   m_verts;
    std::vector<uint32_t>   m_indices;
};

} // namespace Engine
