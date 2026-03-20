#pragma once
#include <cstdint>
#include <vector>
#include <cmath>

namespace PCG {

struct HeightField {
    int32_t            width  = 0;
    int32_t            depth  = 0;
    std::vector<float> data;

    void Resize(int32_t w, int32_t d) {
        width=w; depth=d; data.assign((size_t)w*d, 0.0f);
    }
    float  GetHeight(int32_t x, int32_t z) const {
        if (x<0||x>=width||z<0||z>=depth) return 0.0f;
        return data[(size_t)z*width+x];
    }
    void SetHeight(int32_t x, int32_t z, float v) {
        if (x<0||x>=width||z<0||z>=depth) return;
        data[(size_t)z*width+x]=v;
    }
    void Normalize();
    void AddNoise(uint64_t seed, float amplitude, float frequency);
};

class HeightFieldMesher {
public:
    void Build(const HeightField& field, float uvScale = 1.0f);

    const std::vector<float>&    GetVertices() const { return m_vertices; }
    const std::vector<uint32_t>& GetIndices()  const { return m_indices; }
    const std::vector<float>&    GetNormals()  const { return m_normals; }

    size_t VertexCount()   const { return m_vertices.size() / 3; }
    size_t TriangleCount() const { return m_indices.size()  / 3; }
    void   Clear();

private:
    std::vector<float>    m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<float>    m_normals;

    void computeNormals(const HeightField& field);
};

} // namespace PCG
