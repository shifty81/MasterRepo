#include "PCG/World/HeightField.h"
#include <algorithm>
#include <cmath>

namespace PCG {

void HeightField::Normalize() {
    if (data.empty()) return;
    float mn = *std::min_element(data.begin(), data.end());
    float mx = *std::max_element(data.begin(), data.end());
    float range = mx - mn;
    if (range < 1e-6f) return;
    for (auto& v : data) v = (v - mn) / range;
}

void HeightField::AddNoise(uint64_t seed, float amplitude, float frequency) {
    uint64_t s = seed ^ 0x9e3779b97f4a7c15ULL;
    for (int32_t z = 0; z < depth; ++z) {
        for (int32_t x = 0; x < width; ++x) {
            s ^= s << 13; s ^= s >> 7; s ^= s << 17;
            float n = (float)(s & 0xffff) / 65535.0f * 2.0f - 1.0f;
            float wave = std::sin(x * frequency) * std::cos(z * frequency);
            data[(size_t)z * width + x] += (n * 0.5f + wave * 0.5f) * amplitude;
        }
    }
}

void HeightFieldMesher::Build(const HeightField& field, float uvScale) {
    Clear();
    int32_t w = field.width;
    int32_t d = field.depth;
    if (w < 2 || d < 2) return;

    m_vertices.reserve((size_t)w * d * 3);
    for (int32_t z = 0; z < d; ++z) {
        for (int32_t x = 0; x < w; ++x) {
            m_vertices.push_back((float)x);
            m_vertices.push_back(field.GetHeight(x, z));
            m_vertices.push_back((float)z);
        }
    }

    m_indices.reserve((size_t)(w-1)*(d-1)*6);
    for (int32_t z = 0; z < d-1; ++z) {
        for (int32_t x = 0; x < w-1; ++x) {
            uint32_t tl = (uint32_t)(z*w + x);
            uint32_t tr = tl + 1;
            uint32_t bl = (uint32_t)((z+1)*w + x);
            uint32_t br = bl + 1;
            m_indices.push_back(tl); m_indices.push_back(bl); m_indices.push_back(tr);
            m_indices.push_back(tr); m_indices.push_back(bl); m_indices.push_back(br);
        }
    }
    computeNormals(field);
}

void HeightFieldMesher::computeNormals(const HeightField& field) {
    size_t vtxCount = VertexCount();
    m_normals.assign(vtxCount * 3, 0.0f);
    int32_t w = field.width;
    int32_t d = field.depth;
    for (int32_t z = 0; z < d; ++z) {
        for (int32_t x = 0; x < w; ++x) {
            float L = field.GetHeight(x-1, z);
            float R = field.GetHeight(x+1, z);
            float D = field.GetHeight(x, z-1);
            float U = field.GetHeight(x, z+1);
            float nx = L - R;
            float ny = 2.0f;
            float nz = D - U;
            float len = std::sqrt(nx*nx + ny*ny + nz*nz);
            if (len > 1e-6f) { nx/=len; ny/=len; nz/=len; }
            size_t idx = (size_t)(z*w+x)*3;
            m_normals[idx]=nx; m_normals[idx+1]=ny; m_normals[idx+2]=nz;
        }
    }
}

void HeightFieldMesher::Clear() {
    m_vertices.clear(); m_indices.clear(); m_normals.clear();
}

} // namespace PCG
