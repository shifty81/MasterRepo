#pragma once
/**
 * @file WireframeRenderer.h
 * @brief CPU wireframe overlay: extract edges from triangle mesh, depth-bias, colour, thickness.
 *
 * Features:
 *   - ExtractEdges(vertices, indices) → EdgeList (deduplicated)
 *   - AddMesh(meshId, vertices, indices, transform[16])
 *   - RemoveMesh(meshId)
 *   - UpdateTransform(meshId, transform[16])
 *   - GetLines() → vector of LineSegment {start[3], end[3]}
 *   - SetColour(r,g,b,a)
 *   - SetDepthBias(bias) — offset to prevent z-fighting
 *   - SetThickness(px)
 *   - SetWireframeMode(bool) — toggle all vs selected only
 *   - Supports multiple registered meshes
 *   - Bake/Unbake: pre-transform all edges into world space for fast retrieval
 */

#include <cstdint>
#include <vector>

namespace Engine {

struct WireEdge { float a[3]{}; float b[3]{}; };

class WireframeRenderer {
public:
    WireframeRenderer();
    ~WireframeRenderer();

    void Init    ();
    void Shutdown();

    // Mesh management
    void AddMesh    (uint32_t id, const float* verts, uint32_t vertCount,
                     const uint32_t* indices, uint32_t idxCount,
                     const float transform[16]=nullptr);
    void RemoveMesh (uint32_t id);
    bool HasMesh    (uint32_t id) const;
    void UpdateTransform(uint32_t id, const float transform[16]);

    // Static edge extraction
    static std::vector<WireEdge> ExtractEdges(const float* verts, uint32_t vertCount,
                                               const uint32_t* indices, uint32_t idxCount);

    // Render data
    const std::vector<WireEdge>& GetLines() const;
    void Bake();

    // Visual settings
    void SetColour    (float r, float g, float b, float a=1.f);
    void GetColour    (float& r, float& g, float& b, float& a) const;
    void SetDepthBias (float bias);
    void SetThickness (float px);
    void SetEnabled   (bool e);
    bool IsEnabled    () const;

    void Clear();

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
