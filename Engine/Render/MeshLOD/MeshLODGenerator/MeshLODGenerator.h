#pragma once
/**
 * @file MeshLODGenerator.h
 * @brief Offline LOD mesh simplification: edge-collapse, % vertex reduction, per-submesh levels.
 *
 * Features:
 *   - InputMesh: vertices (float pos[3] array), indices (uint32 triangle list)
 *   - GenerateLOD(inputMesh, targetRatio) → LODMesh (simplified vertex/index arrays)
 *   - GenerateLODChain(inputMesh, ratios[]) → vector of LODMesh levels
 *   - Edge-collapse with quadric error metric
 *   - Attribute-aware: preserves UV seams and hard normals (seam threshold)
 *   - Per-submesh: each submesh reduced independently
 *   - GetStats(level) → {vertexCount, triangleCount, reductionRatio}
 *   - BorderPreservation: flag to lock boundary edges
 *   - Weld(threshold): merge near-duplicate vertices before simplification
 */

#include <cstdint>
#include <vector>

namespace Engine {

struct LODVertex {
    float pos[3]{};
    float uv [2]{};
    float nor[3]{};
};

struct LODMesh {
    std::vector<LODVertex>  vertices;
    std::vector<uint32_t>   indices;
    float                   actualRatio{1.f};
};

struct LODStats {
    uint32_t vertexCount   {0};
    uint32_t triangleCount {0};
    float    reductionRatio{1.f};
};

struct LODInputMesh {
    std::vector<LODVertex> vertices;
    std::vector<uint32_t>  indices;
};

class MeshLODGenerator {
public:
    MeshLODGenerator();
    ~MeshLODGenerator();

    void Init    ();
    void Shutdown();

    // Settings
    void SetPreserveBorders(bool e);
    void SetSeamThreshold  (float t);
    void SetWeldThreshold  (float t);

    // Generation
    LODMesh              GenerateLOD      (const LODInputMesh& src, float targetRatio);
    std::vector<LODMesh> GenerateLODChain (const LODInputMesh& src,
                                           const std::vector<float>& ratios);

    // Stats
    LODStats GetStats(const LODMesh& mesh, const LODInputMesh& original) const;

    // Utility
    LODInputMesh Weld   (const LODInputMesh& src, float threshold=1e-4f) const;
    LODInputMesh Recalc (const LODInputMesh& src) const; ///< recalculate normals

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
