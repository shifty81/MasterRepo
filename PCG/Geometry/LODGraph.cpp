#include "PCG/Geometry/LODGraph.h"
#include <algorithm>
#include <cstdlib>

namespace PCG {

void LODGraph::AddSourceMesh(const std::vector<float>& vertices,
                              const std::vector<uint32_t>& indices) {
    m_srcVertices = vertices;
    m_srcIndices  = indices;
    m_levels.clear();
}

void LODGraph::BakeLOD(uint8_t level, float targetPolyReduction) {
    LODLevel lod;
    lod.level = level;
    uint32_t srcTris = (uint32_t)(m_srcIndices.size() / 3);
    lod.targetPolyCount = std::max(1u,
        (uint32_t)(srcTris * (1.0f - std::min(1.0f, targetPolyReduction))));
    // Simple decimation: keep every Nth triangle
    uint32_t step = srcTris > 0 ? std::max(1u, srcTris / lod.targetPolyCount) : 1;
    lod.meshData = m_srcVertices;
    for (uint32_t t = 0; t < srcTris; t += step) {
        lod.indexData.push_back(m_srcIndices[t * 3 + 0]);
        lod.indexData.push_back(m_srcIndices[t * 3 + 1]);
        lod.indexData.push_back(m_srcIndices[t * 3 + 2]);
    }
    m_levels.push_back(std::move(lod));
}

const LODLevel* LODGraph::GetLOD(uint8_t level) const {
    for (auto& l : m_levels) if (l.level == level) return &l;
    return nullptr;
}

void LODGraph::Clear() {
    m_srcVertices.clear();
    m_srcIndices.clear();
    m_levels.clear();
}

} // namespace PCG
