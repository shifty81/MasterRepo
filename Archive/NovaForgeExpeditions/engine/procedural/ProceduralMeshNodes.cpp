#include "ProceduralMeshNodes.h"
#include <cmath>
#include <unordered_map>

namespace atlas::procedural {

static const float PI = 3.14159265358979323846f;

// Compute flat normal for a triangle
static void ComputeNormal(float ax, float ay, float az,
                           float bx, float by, float bz,
                           float cx, float cy, float cz,
                           float& nx, float& ny, float& nz) {
    float ux = bx - ax, uy = by - ay, uz = bz - az;
    float vx = cx - ax, vy = cy - ay, vz = cz - az;
    nx = uy * vz - uz * vy;
    ny = uz * vx - ux * vz;
    nz = ux * vy - uy * vx;
    float len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (len > 0.0f) { nx /= len; ny /= len; nz /= len; }
}

MeshData GenerateCube(float size) {
    MeshData mesh;
    float h = size / 2.0f;

    // 8 unique vertices
    float verts[] = {
        -h, -h, -h,   h, -h, -h,   h,  h, -h,  -h,  h, -h,
        -h, -h,  h,   h, -h,  h,   h,  h,  h,  -h,  h,  h
    };
    mesh.vertices.assign(verts, verts + 24);

    // Per-vertex normals (averaged from adjacent faces)
    mesh.normals.resize(24);
    for (size_t i = 0; i < 24; i += 3) {
        float x = mesh.vertices[i], y = mesh.vertices[i + 1], z = mesh.vertices[i + 2];
        float len = std::sqrt(x * x + y * y + z * z);
        if (len > 0.0f) {
            mesh.normals[i] = x / len;
            mesh.normals[i + 1] = y / len;
            mesh.normals[i + 2] = z / len;
        } else {
            mesh.normals[i] = 0.0f;
            mesh.normals[i + 1] = 1.0f;
            mesh.normals[i + 2] = 0.0f;
        }
    }

    // 12 triangles (36 indices)
    uint32_t idx[] = {
        // front
        4, 5, 6,  4, 6, 7,
        // back
        1, 0, 3,  1, 3, 2,
        // top
        7, 6, 2,  7, 2, 3,
        // bottom
        0, 1, 5,  0, 5, 4,
        // right
        5, 1, 2,  5, 2, 6,
        // left
        0, 4, 7,  0, 7, 3
    };
    mesh.indices.assign(idx, idx + 36);

    return mesh;
}

MeshData GeneratePlane(float size) {
    MeshData mesh;
    float h = size / 2.0f;

    // 4 vertices in XZ plane at y=0
    float verts[] = {
        -h, 0.0f, -h,
         h, 0.0f, -h,
         h, 0.0f,  h,
        -h, 0.0f,  h
    };
    mesh.vertices.assign(verts, verts + 12);

    // Normals pointing up
    float norms[] = {
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };
    mesh.normals.assign(norms, norms + 12);

    // 2 triangles
    uint32_t idx[] = { 0, 1, 2, 0, 2, 3 };
    mesh.indices.assign(idx, idx + 6);

    return mesh;
}

MeshData GenerateSphere(float size, int segments) {
    MeshData mesh;
    float radius = size / 2.0f;
    int rings = segments;
    int sectors = segments;

    for (int r = 0; r <= rings; ++r) {
        float phi = PI * static_cast<float>(r) / static_cast<float>(rings);
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        for (int s = 0; s <= sectors; ++s) {
            float theta = 2.0f * PI * static_cast<float>(s) / static_cast<float>(sectors);
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            float nx = sinPhi * cosTheta;
            float ny = cosPhi;
            float nz = sinPhi * sinTheta;

            mesh.vertices.push_back(radius * nx);
            mesh.vertices.push_back(radius * ny);
            mesh.vertices.push_back(radius * nz);

            mesh.normals.push_back(nx);
            mesh.normals.push_back(ny);
            mesh.normals.push_back(nz);
        }
    }

    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            uint32_t a = static_cast<uint32_t>(r * (sectors + 1) + s);
            uint32_t b = a + static_cast<uint32_t>(sectors + 1);
            uint32_t c = a + 1;
            uint32_t d = b + 1;

            mesh.indices.push_back(a);
            mesh.indices.push_back(b);
            mesh.indices.push_back(c);

            mesh.indices.push_back(c);
            mesh.indices.push_back(b);
            mesh.indices.push_back(d);
        }
    }

    return mesh;
}

MeshData GenerateCylinder(float size, int segments) {
    MeshData mesh;
    float radius = size / 2.0f;
    float halfHeight = size / 2.0f;

    // Bottom ring (index 0..segments-1), then top ring (segments..2*segments-1)
    for (int i = 0; i < segments; ++i) {
        float theta = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
        float ct = std::cos(theta);
        float st = std::sin(theta);

        // bottom vertex
        mesh.vertices.push_back(radius * ct);
        mesh.vertices.push_back(-halfHeight);
        mesh.vertices.push_back(radius * st);
        mesh.normals.push_back(ct);
        mesh.normals.push_back(0.0f);
        mesh.normals.push_back(st);

        // top vertex
        mesh.vertices.push_back(radius * ct);
        mesh.vertices.push_back(halfHeight);
        mesh.vertices.push_back(radius * st);
        mesh.normals.push_back(ct);
        mesh.normals.push_back(0.0f);
        mesh.normals.push_back(st);
    }

    // Side triangles connecting bottom and top rings
    for (int i = 0; i < segments; ++i) {
        uint32_t b0 = static_cast<uint32_t>(i * 2);
        uint32_t t0 = b0 + 1;
        uint32_t b1 = static_cast<uint32_t>(((i + 1) % segments) * 2);
        uint32_t t1 = b1 + 1;

        mesh.indices.push_back(b0);
        mesh.indices.push_back(b1);
        mesh.indices.push_back(t0);

        mesh.indices.push_back(t0);
        mesh.indices.push_back(b1);
        mesh.indices.push_back(t1);
    }

    // Bottom cap (fan from center)
    uint32_t bcIdx = static_cast<uint32_t>(mesh.VertexCount());
    mesh.vertices.push_back(0.0f);
    mesh.vertices.push_back(-halfHeight);
    mesh.vertices.push_back(0.0f);
    mesh.normals.push_back(0.0f);
    mesh.normals.push_back(-1.0f);
    mesh.normals.push_back(0.0f);

    for (int i = 0; i < segments; ++i) {
        uint32_t b0 = static_cast<uint32_t>(i * 2);
        uint32_t b1 = static_cast<uint32_t>(((i + 1) % segments) * 2);
        mesh.indices.push_back(bcIdx);
        mesh.indices.push_back(b1);
        mesh.indices.push_back(b0);
    }

    // Top cap (fan from center)
    uint32_t tcIdx = static_cast<uint32_t>(mesh.VertexCount());
    mesh.vertices.push_back(0.0f);
    mesh.vertices.push_back(halfHeight);
    mesh.vertices.push_back(0.0f);
    mesh.normals.push_back(0.0f);
    mesh.normals.push_back(1.0f);
    mesh.normals.push_back(0.0f);

    for (int i = 0; i < segments; ++i) {
        uint32_t t0 = static_cast<uint32_t>(i * 2 + 1);
        uint32_t t1 = static_cast<uint32_t>(((i + 1) % segments) * 2 + 1);
        mesh.indices.push_back(tcIdx);
        mesh.indices.push_back(t0);
        mesh.indices.push_back(t1);
    }

    return mesh;
}

MeshData SubdivideMesh(const MeshData& input) {
    MeshData result;
    if (input.indices.empty()) return input;

    result.vertices = input.vertices;
    result.normals = input.normals;

    // Edge midpoint cache: key = packed pair of sorted vertex indices
    std::unordered_map<uint64_t, uint32_t> midpointCache;

    auto getMidpoint = [&](uint32_t a, uint32_t b) -> uint32_t {
        uint32_t lo = (a < b) ? a : b;
        uint32_t hi = (a < b) ? b : a;
        uint64_t key = (static_cast<uint64_t>(lo) << 32) | hi;

        auto it = midpointCache.find(key);
        if (it != midpointCache.end()) return it->second;

        uint32_t idx = static_cast<uint32_t>(result.vertices.size() / 3);
        for (int c = 0; c < 3; ++c) {
            result.vertices.push_back(
                (result.vertices[a * 3 + c] + result.vertices[b * 3 + c]) * 0.5f);
            result.normals.push_back(
                (result.normals[a * 3 + c] + result.normals[b * 3 + c]) * 0.5f);
        }

        // Renormalize the interpolated normal
        size_t ni = idx * 3;
        float nx = result.normals[ni], ny = result.normals[ni + 1], nz = result.normals[ni + 2];
        float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (len > 0.0f) {
            result.normals[ni] = nx / len;
            result.normals[ni + 1] = ny / len;
            result.normals[ni + 2] = nz / len;
        }

        midpointCache[key] = idx;
        return idx;
    };

    for (size_t i = 0; i + 2 < input.indices.size(); i += 3) {
        uint32_t v0 = input.indices[i];
        uint32_t v1 = input.indices[i + 1];
        uint32_t v2 = input.indices[i + 2];

        uint32_t m01 = getMidpoint(v0, v1);
        uint32_t m12 = getMidpoint(v1, v2);
        uint32_t m20 = getMidpoint(v2, v0);

        result.indices.push_back(v0);  result.indices.push_back(m01); result.indices.push_back(m20);
        result.indices.push_back(m01); result.indices.push_back(v1);  result.indices.push_back(m12);
        result.indices.push_back(m20); result.indices.push_back(m12); result.indices.push_back(v2);
        result.indices.push_back(m01); result.indices.push_back(m12); result.indices.push_back(m20);
    }

    return result;
}

MeshData ApplyNoiseDisplacement(const MeshData& input, float amplitude, uint64_t seed) {
    MeshData result = input;

    // Xorshift64 deterministic RNG — seed mixing constant from Knuth's MMIX LCG
    uint64_t state = seed ^ 0x5DEECE66DULL;
    if (state == 0) state = 1;

    auto xorshift = [&]() -> float {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return static_cast<float>(state & 0xFFFFu) / 65535.0f;
    };

    for (size_t i = 0; i + 2 < result.vertices.size(); i += 3) {
        float disp = (xorshift() * 2.0f - 1.0f) * amplitude;
        result.vertices[i]     += result.normals[i]     * disp;
        result.vertices[i + 1] += result.normals[i + 1] * disp;
        result.vertices[i + 2] += result.normals[i + 2] * disp;
    }

    return result;
}

}
