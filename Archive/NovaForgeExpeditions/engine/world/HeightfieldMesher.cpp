#include "HeightfieldMesher.h"
#include <cmath>

namespace atlas::world {

MeshData HeightfieldMesher::BuildMesh(const Heightfield& hf, int lod) {
    MeshData mesh;

    if (hf.size < 2 || hf.data.empty()) return mesh;

    int step = 1 << lod;
    if (step >= hf.size) step = hf.size - 1;

    int gridSize = (hf.size - 1) / step;
    if (gridSize < 1) gridSize = 1;

    int vertsPerSide = gridSize + 1;
    size_t vertCount = static_cast<size_t>(vertsPerSide) * vertsPerSide;

    mesh.vertices.resize(vertCount * 3);
    mesh.normals.resize(vertCount * 3, 0.0f);
    mesh.uvs.resize(vertCount * 2);

    // Generate vertices and UVs
    for (int iz = 0; iz < vertsPerSide; ++iz) {
        for (int ix = 0; ix < vertsPerSide; ++ix) {
            int sx = ix * step;
            int sz = iz * step;
            if (sx >= hf.size) sx = hf.size - 1;
            if (sz >= hf.size) sz = hf.size - 1;

            size_t vi = (static_cast<size_t>(iz) * vertsPerSide + ix) * 3;
            mesh.vertices[vi]     = static_cast<float>(sx) * hf.scale;
            mesh.vertices[vi + 1] = hf.At(sx, sz);
            mesh.vertices[vi + 2] = static_cast<float>(sz) * hf.scale;

            size_t ui = (static_cast<size_t>(iz) * vertsPerSide + ix) * 2;
            mesh.uvs[ui]     = static_cast<float>(ix) / static_cast<float>(gridSize);
            mesh.uvs[ui + 1] = static_cast<float>(iz) / static_cast<float>(gridSize);
        }
    }

    // Generate indices (two triangles per quad)
    mesh.indices.reserve(static_cast<size_t>(gridSize) * gridSize * 6);

    for (int iz = 0; iz < gridSize; ++iz) {
        for (int ix = 0; ix < gridSize; ++ix) {
            uint32_t tl = static_cast<uint32_t>(iz * vertsPerSide + ix);
            uint32_t tr = tl + 1;
            uint32_t bl = static_cast<uint32_t>((iz + 1) * vertsPerSide + ix);
            uint32_t br = bl + 1;

            mesh.indices.push_back(tl);
            mesh.indices.push_back(bl);
            mesh.indices.push_back(tr);

            mesh.indices.push_back(tr);
            mesh.indices.push_back(bl);
            mesh.indices.push_back(br);
        }
    }

    ComputeNormals(mesh);
    return mesh;
}

void HeightfieldMesher::ComputeNormals(MeshData& mesh) {
    size_t vertCount = mesh.vertices.size() / 3;
    mesh.normals.assign(vertCount * 3, 0.0f);

    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        uint32_t i0 = mesh.indices[i];
        uint32_t i1 = mesh.indices[i + 1];
        uint32_t i2 = mesh.indices[i + 2];

        float ax = mesh.vertices[i0 * 3],     ay = mesh.vertices[i0 * 3 + 1], az = mesh.vertices[i0 * 3 + 2];
        float bx = mesh.vertices[i1 * 3],     by = mesh.vertices[i1 * 3 + 1], bz = mesh.vertices[i1 * 3 + 2];
        float cx = mesh.vertices[i2 * 3],     cy = mesh.vertices[i2 * 3 + 1], cz = mesh.vertices[i2 * 3 + 2];

        float ux = bx - ax, uy = by - ay, uz = bz - az;
        float vx = cx - ax, vy = cy - ay, vz = cz - az;

        float nx = uy * vz - uz * vy;
        float ny = uz * vx - ux * vz;
        float nz = ux * vy - uy * vx;

        mesh.normals[i0 * 3]     += nx; mesh.normals[i0 * 3 + 1] += ny; mesh.normals[i0 * 3 + 2] += nz;
        mesh.normals[i1 * 3]     += nx; mesh.normals[i1 * 3 + 1] += ny; mesh.normals[i1 * 3 + 2] += nz;
        mesh.normals[i2 * 3]     += nx; mesh.normals[i2 * 3 + 1] += ny; mesh.normals[i2 * 3 + 2] += nz;
    }

    // Normalize
    for (size_t i = 0; i < vertCount; ++i) {
        float nx = mesh.normals[i * 3];
        float ny = mesh.normals[i * 3 + 1];
        float nz = mesh.normals[i * 3 + 2];
        float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (len > 0.0f) {
            mesh.normals[i * 3]     /= len;
            mesh.normals[i * 3 + 1] /= len;
            mesh.normals[i * 3 + 2] /= len;
        }
    }
}

}
