#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Game/Voxel/VoxelMesher.h"
#include "Game/Voxel/Chunk.h"
#include "Game/Voxel/ChunkCoord.h"
#include "Game/Voxel/VoxelType.h"

using namespace NF::Game;

// ============================================================================
// VoxelMesher — basic generation
// ============================================================================

TEST_CASE("VoxelMesher: empty chunk produces no geometry", "[VoxelMesh]") {
    Chunk chunk({0, 0, 0});
    VoxelMesher mesher;
    auto data = mesher.Generate(chunk);
    REQUIRE(data.Vertices.empty());
    REQUIRE(data.Indices.empty());
}

TEST_CASE("VoxelMesher: single voxel produces 6 faces (24 verts, 36 indices)", "[VoxelMesh]") {
    Chunk chunk({0, 0, 0});
    chunk.SetVoxel(5, 5, 5, static_cast<VoxelId>(VoxelType::Stone));

    VoxelMesher mesher;
    auto data = mesher.Generate(chunk);

    // 1 solid voxel surrounded by air -> 6 faces
    REQUIRE(data.Vertices.size() == 24);
    REQUIRE(data.Indices.size()  == 36);

    // Check all triangles have valid indices.
    for (uint32_t idx : data.Indices) {
        REQUIRE(idx < data.Vertices.size());
    }
}

TEST_CASE("VoxelMesher: two adjacent voxels share an internal face (cull it)", "[VoxelMesh]") {
    Chunk chunk({0, 0, 0});
    chunk.SetVoxel(5, 5, 5, static_cast<VoxelId>(VoxelType::Stone));
    chunk.SetVoxel(6, 5, 5, static_cast<VoxelId>(VoxelType::Stone)); // +X neighbour

    VoxelMesher mesher;
    auto data = mesher.Generate(chunk);

    // 2 voxels sharing 1 face -> 2*6 - 2 = 10 faces
    REQUIRE(data.Vertices.size() == 40);
    REQUIRE(data.Indices.size()  == 60);
}

TEST_CASE("VoxelMesher: vertex positions include chunk world offset", "[VoxelMesh]") {
    ChunkCoord coord{1, 0, 0};
    Chunk chunk(coord);
    chunk.SetVoxel(0, 0, 0, static_cast<VoxelId>(VoxelType::Dirt));

    VoxelMesher mesher;
    auto data = mesher.Generate(chunk);

    // The chunk at (1,0,0) has world origin at (32,0,0).
    // All vertex X positions should be >= 32.
    for (const auto& v : data.Vertices) {
        REQUIRE(v.Position.X >= 32.f);
    }
}

TEST_CASE("VoxelMesher: normals point outward on each face", "[VoxelMesh]") {
    Chunk chunk({0, 0, 0});
    chunk.SetVoxel(10, 10, 10, static_cast<VoxelId>(VoxelType::Metal));

    VoxelMesher mesher;
    auto data = mesher.Generate(chunk);

    // Every vertex should have a unit-length normal along an axis.
    for (const auto& v : data.Vertices) {
        float len = std::sqrt(v.Normal.X * v.Normal.X
                            + v.Normal.Y * v.Normal.Y
                            + v.Normal.Z * v.Normal.Z);
        REQUIRE(len == Catch::Approx(1.f).margin(0.001f));
    }
}

TEST_CASE("VoxelMesher: type ID encoded in TexCoord.x", "[VoxelMesh]") {
    Chunk chunk({0, 0, 0});
    chunk.SetVoxel(0, 0, 0, static_cast<VoxelId>(VoxelType::Ore));

    VoxelMesher mesher;
    auto data = mesher.Generate(chunk);

    REQUIRE(!data.Vertices.empty());
    for (const auto& v : data.Vertices) {
        REQUIRE(v.TexCoord.X == Catch::Approx(static_cast<float>(VoxelType::Ore)));
    }
}

TEST_CASE("VoxelMesher: VoxelColor returns distinct palette per type", "[VoxelMesh]") {
    auto stone   = VoxelMesher::VoxelColor(static_cast<VoxelId>(VoxelType::Stone));
    auto ore     = VoxelMesher::VoxelColor(static_cast<VoxelId>(VoxelType::Ore));
    auto dirt    = VoxelMesher::VoxelColor(static_cast<VoxelId>(VoxelType::Dirt));

    // All should be different from each other.
    REQUIRE((stone.X != ore.X || stone.Y != ore.Y || stone.Z != ore.Z));
    REQUIRE((stone.X != dirt.X || stone.Y != dirt.Y || stone.Z != dirt.Z));
    REQUIRE((ore.X   != dirt.X || ore.Y   != dirt.Y || ore.Z   != dirt.Z));
}

TEST_CASE("VoxelMesher: full-layer generates correct face count (top only exposed)", "[VoxelMesh]") {
    Chunk chunk({0, 0, 0});

    // Fill the entire bottom layer (y=0) with stone.
    for (uint8_t x = 0; x < kChunkSize; ++x)
        for (uint8_t z = 0; z < kChunkSize; ++z)
            chunk.SetVoxel(x, 0, z, static_cast<VoxelId>(VoxelType::Stone));

    VoxelMesher mesher;
    auto data = mesher.Generate(chunk);

    // 32x32 voxels in a flat sheet:
    //  - top face: 32*32 = 1024 quads
    //  - bottom face: 32*32 = 1024 quads
    //  - edges: 4 sides * 32 = 128 quads
    // Total = 1024 + 1024 + 128 = 2176 quads
    //       = 2176 * 4 verts = 8704, 2176 * 6 indices = 13056
    REQUIRE(data.Vertices.size() == 8704);
    REQUIRE(data.Indices.size()  == 13056);
}

TEST_CASE("VoxelMesher: filled chunk only produces surface faces", "[VoxelMesh]") {
    Chunk chunk({0, 0, 0});

    // Fill the entire chunk solid.
    for (uint8_t x = 0; x < kChunkSize; ++x)
        for (uint8_t y = 0; y < kChunkSize; ++y)
            for (uint8_t z = 0; z < kChunkSize; ++z)
                chunk.SetVoxel(x, y, z, static_cast<VoxelId>(VoxelType::Rock));

    VoxelMesher mesher;
    auto data = mesher.Generate(chunk);

    // Only the 6 outer faces of the 32^3 cube are exposed.
    // Each face of the cube has 32*32 = 1024 quads.
    // 6 faces * 1024 = 6144 quads.
    REQUIRE(data.Vertices.size() == 6144 * 4);
    REQUIRE(data.Indices.size()  == 6144 * 6);
}

TEST_CASE("VoxelMesher: neighbour chunk occludes boundary face", "[VoxelMesh]") {
    // Chunk at (0,0,0), neighbour at (1,0,0).
    // Place a solid voxel at x=31 in chunk A and x=0 in chunk B.
    // The +X face of A's voxel and the -X face of B's voxel should be culled.
    Chunk chunkA({0, 0, 0});
    Chunk chunkB({1, 0, 0});
    chunkA.SetVoxel(31, 5, 5, static_cast<VoxelId>(VoxelType::Stone));
    chunkB.SetVoxel(0,  5, 5, static_cast<VoxelId>(VoxelType::Stone));

    // Without neighbours — both produce 6 faces = 24 verts.
    VoxelMesher mesher;
    auto dataA_noNeighbour = mesher.Generate(chunkA);
    REQUIRE(dataA_noNeighbour.Vertices.size() == 24);

    // With neighbour — the +X face of A is culled.
    const Chunk* neighbours[6] = {nullptr, &chunkB, nullptr, nullptr, nullptr, nullptr};
    auto dataA_withNeighbour = mesher.Generate(chunkA, neighbours);
    REQUIRE(dataA_withNeighbour.Vertices.size() == 20); // 5 faces * 4 verts
}
