#pragma once
#include <cstdint>
#include <vector>

namespace PCG {

struct LODLevel {
    uint8_t              level = 0;
    uint32_t             targetPolyCount = 0;
    std::vector<float>   meshData;
    std::vector<uint32_t> indexData;
};

class LODGraph {
public:
    void AddSourceMesh(const std::vector<float>& vertices,
                       const std::vector<uint32_t>& indices);
    void BakeLOD(uint8_t level, float targetPolyReduction);
    const LODLevel* GetLOD(uint8_t level) const;
    size_t LevelCount() const { return m_levels.size(); }
    void Clear();

private:
    std::vector<float>    m_srcVertices;
    std::vector<uint32_t> m_srcIndices;
    std::vector<LODLevel> m_levels;
};

} // namespace PCG
