#include "Game/Voxel/VoxelMesher.h"
#include "Core/Math/Vector.h"

namespace NF::Game {

// ---------------------------------------------------------------------------
// Per-voxel-type colour palette
// ---------------------------------------------------------------------------

Vector3 VoxelMesher::VoxelColor(VoxelId id) noexcept {
    switch (static_cast<VoxelType>(id)) {
        case VoxelType::Stone:   return {0.55f, 0.55f, 0.55f}; // grey
        case VoxelType::Ore:     return {0.85f, 0.65f, 0.25f}; // gold
        case VoxelType::Dirt:    return {0.45f, 0.30f, 0.15f}; // brown
        case VoxelType::Rock:    return {0.40f, 0.40f, 0.42f}; // dark grey
        case VoxelType::Metal:   return {0.70f, 0.72f, 0.75f}; // silver
        case VoxelType::Ice:     return {0.70f, 0.85f, 0.95f}; // light blue
        case VoxelType::Organic: return {0.30f, 0.55f, 0.20f}; // green
        default:                 return {1.00f, 0.00f, 1.00f}; // magenta (debug)
    }
}

// ---------------------------------------------------------------------------
// Face geometry tables
// ---------------------------------------------------------------------------

// Each face has 4 vertex offsets (relative to voxel origin) and 1 normal.
// Winding order is counter-clockwise when viewed from outside the face.
// Vertex offsets are 0 or 1 along each axis, defining a unit-cube face.

struct FaceInfo {
    float verts[4][3]; // 4 corners (x, y, z offsets from voxel origin)
    float normal[3];   // outward-pointing face normal
    int   axis;        // axis index: 0 = X, 1 = Y, 2 = Z
    int   dir;         // direction along axis: -1 or +1
};

// Face order matches neighbour array convention used by IsTransparent():
//   0 = -X (left),  1 = +X (right),
//   2 = -Y (bottom), 3 = +Y (top),
//   4 = -Z (back),   5 = +Z (front)
static constexpr FaceInfo kFaces[6] = {
    // -X face (left)
    {{{0,0,0}, {0,0,1}, {0,1,1}, {0,1,0}}, {-1,0,0}, 0, -1},
    // +X face (right)
    {{{1,0,1}, {1,0,0}, {1,1,0}, {1,1,1}}, { 1,0,0}, 0, +1},
    // -Y face (bottom)
    {{{0,0,0}, {1,0,0}, {1,0,1}, {0,0,1}}, {0,-1,0}, 1, -1},
    // +Y face (top)
    {{{0,1,1}, {1,1,1}, {1,1,0}, {0,1,0}}, {0, 1,0}, 1, +1},
    // -Z face (back)
    {{{1,0,0}, {0,0,0}, {0,1,0}, {1,1,0}}, {0,0,-1}, 2, -1},
    // +Z face (front)
    {{{0,0,1}, {1,0,1}, {1,1,1}, {0,1,1}}, {0,0, 1}, 2, +1},
};

// ---------------------------------------------------------------------------
// Neighbour access helper
// ---------------------------------------------------------------------------

/// Return true if the neighbour voxel in the given direction is transparent
/// (Air or out of bounds).  Neighbour chunks are checked when the local
/// coordinate wraps across a chunk boundary.
static bool IsTransparent(const Chunk& chunk, const Chunk* neighbours[6],
                          int x, int y, int z, int face) noexcept
{
    const auto& fi = kFaces[face];
    int nx = x + fi.normal[0]; // fi.dir along fi.axis
    int ny = y + fi.normal[1];
    int nz = z + fi.normal[2];

    // Inside the same chunk?
    if (nx >= 0 && nx < kChunkSize &&
        ny >= 0 && ny < kChunkSize &&
        nz >= 0 && nz < kChunkSize) {
        VoxelId v = chunk.GetVoxel(static_cast<uint8_t>(nx),
                                    static_cast<uint8_t>(ny),
                                    static_cast<uint8_t>(nz));
        return v == static_cast<VoxelId>(VoxelType::Air);
    }

    // Crossed a chunk boundary — use the neighbour chunk if available.
    if (neighbours[face]) {
        // Wrap coordinate into [0, kChunkSize)
        int lx = (nx % kChunkSize + kChunkSize) % kChunkSize;
        int ly = (ny % kChunkSize + kChunkSize) % kChunkSize;
        int lz = (nz % kChunkSize + kChunkSize) % kChunkSize;
        VoxelId v = neighbours[face]->GetVoxel(static_cast<uint8_t>(lx),
                                                static_cast<uint8_t>(ly),
                                                static_cast<uint8_t>(lz));
        return v == static_cast<VoxelId>(VoxelType::Air);
    }

    // No neighbour data — treat boundary as open air.
    return true;
}

// ---------------------------------------------------------------------------
// Mesh generation
// ---------------------------------------------------------------------------

MeshData VoxelMesher::Generate(const Chunk& chunk,
                                const Chunk* neighbours[6]) const
{
    MeshData data;
    if (chunk.IsEmpty()) return data;

    // Reserve a rough estimate to reduce re-allocations.
    const int32_t solidCount = chunk.CountSolid();
    data.Vertices.reserve(static_cast<size_t>(solidCount) * 4);
    data.Indices.reserve(static_cast<size_t>(solidCount) * 6);

    // Chunk world-space origin.
    int32_t ox, oy, oz;
    ChunkOrigin(chunk.GetCoord(), ox, oy, oz);

    for (uint8_t x = 0; x < kChunkSize; ++x) {
        for (uint8_t y = 0; y < kChunkSize; ++y) {
            for (uint8_t z = 0; z < kChunkSize; ++z) {
                const VoxelId id = chunk.GetVoxel(x, y, z);
                if (id == static_cast<VoxelId>(VoxelType::Air))
                    continue;

                const float typeIdx = static_cast<float>(id);

                for (int face = 0; face < 6; ++face) {
                    if (!IsTransparent(chunk, neighbours, x, y, z, face))
                        continue;

                    const auto& fi = kFaces[face];
                    const auto baseIdx = static_cast<uint32_t>(data.Vertices.size());

                    for (int v = 0; v < 4; ++v) {
                        Vertex vert{};
                        vert.Position = {
                            static_cast<float>(ox + x) + fi.verts[v][0],
                            static_cast<float>(oy + y) + fi.verts[v][1],
                            static_cast<float>(oz + z) + fi.verts[v][2]
                        };
                        vert.Normal = {fi.normal[0], fi.normal[1], fi.normal[2]};
                        // Encode the VoxelId in TexCoord.x for shader palette lookup.
                        vert.TexCoord = {typeIdx, 0.f};
                        data.Vertices.push_back(vert);
                    }

                    // Two triangles per face.
                    data.Indices.push_back(baseIdx + 0);
                    data.Indices.push_back(baseIdx + 1);
                    data.Indices.push_back(baseIdx + 2);

                    data.Indices.push_back(baseIdx + 0);
                    data.Indices.push_back(baseIdx + 2);
                    data.Indices.push_back(baseIdx + 3);
                }
            }
        }
    }

    return data;
}

MeshData VoxelMesher::Generate(const Chunk& chunk) const {
    const Chunk* empty[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    return Generate(chunk, empty);
}

} // namespace NF::Game
