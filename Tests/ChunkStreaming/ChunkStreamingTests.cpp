#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Game/Voxel/ChunkStreamer.h"
#include "Game/Voxel/ChunkMap.h"
#include "Game/Voxel/ChunkCoord.h"
#include "Game/Voxel/VoxelType.h"
#include "Game/Voxel/VoxelSerializer.h"
#include "Game/World/GameWorld.h"
#include "Game/App/Orchestrator.h"

#include <thread>
#include <chrono>
#include <filesystem>

using namespace NF::Game;

// ============================================================================
// ChunkStreamer — basic lifecycle
// ============================================================================

TEST_CASE("ChunkStreamer: default-constructed is not initialized", "[ChunkStreaming]") {
    ChunkStreamer streamer;
    REQUIRE_FALSE(streamer.IsInitialized());
    REQUIRE(streamer.PendingCount() == 0);
    REQUIRE(streamer.TotalEvicted() == 0);
    REQUIRE(streamer.TotalGenerated() == 0);
}

TEST_CASE("ChunkStreamer: Init and Shutdown lifecycle", "[ChunkStreaming]") {
    ChunkMap map;
    ChunkStreamConfig cfg;
    cfg.LoadRadius  = 2;
    cfg.UnloadRadius = 3;

    ChunkStreamer streamer;
    streamer.Init(&map, nullptr, cfg, 42);
    REQUIRE(streamer.IsInitialized());
    REQUIRE(streamer.GetConfig().LoadRadius == 2);
    REQUIRE(streamer.GetConfig().UnloadRadius == 3);

    streamer.Shutdown();
    REQUIRE_FALSE(streamer.IsInitialized());
}

// ============================================================================
// ChunkStreamer — streaming radius
// ============================================================================

TEST_CASE("ChunkStreamer: Tick loads chunks within radius", "[ChunkStreaming]") {
    ChunkMap map;
    ChunkStreamConfig cfg;
    cfg.LoadRadius  = 2;
    cfg.UnloadRadius = 4;
    cfg.MaxLoadsPerTick = 100; // allow all at once for this test

    ChunkStreamer streamer;
    streamer.Init(&map, nullptr, cfg, 42);

    // Before tick, no chunks.
    REQUIRE(map.ChunkCount() == 0);

    // Tick at origin to queue generation.
    streamer.Tick({0, 0, 0});

    // Allow background tasks to complete.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Second tick to receive completed chunks.
    streamer.Tick({0, 0, 0});

    // Chunks should be loaded within radius.
    // With LoadRadius=2 and Y in [-1, 1], the sphere of radius 2 should
    // contain at least the chunks at (0,0,0), (±1,0,0), (0,0,±1).
    REQUIRE(map.ChunkCount() > 0);
    REQUIRE(map.HasChunk({0, 0, 0}));
    REQUIRE(map.HasChunk({1, 0, 0}));
    REQUIRE(map.HasChunk({-1, 0, 0}));
    REQUIRE(map.HasChunk({0, 0, 1}));
    REQUIRE(map.HasChunk({0, 0, -1}));

    streamer.Shutdown();
}

TEST_CASE("ChunkStreamer: does not reload existing chunks", "[ChunkStreaming]") {
    ChunkMap map;
    // Pre-populate a chunk.
    map.CreateChunk({0, 0, 0});
    REQUIRE(map.ChunkCount() == 1);

    ChunkStreamConfig cfg;
    cfg.LoadRadius  = 1;
    cfg.UnloadRadius = 3;
    cfg.MaxLoadsPerTick = 100;

    ChunkStreamer streamer;
    streamer.Init(&map, nullptr, cfg, 42);

    streamer.Tick({0, 0, 0});
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    streamer.Tick({0, 0, 0});

    // The pre-existing chunk at origin should still be there.
    REQUIRE(map.HasChunk({0, 0, 0}));

    streamer.Shutdown();
}

// ============================================================================
// ChunkStreamer — eviction
// ============================================================================

TEST_CASE("ChunkStreamer: evicts chunks beyond unload radius", "[ChunkStreaming]") {
    ChunkMap map;
    // Place a chunk far from origin.
    map.CreateChunk({10, 0, 10});
    REQUIRE(map.ChunkCount() == 1);

    ChunkStreamConfig cfg;
    cfg.LoadRadius      = 1;
    cfg.UnloadRadius    = 3;
    cfg.MaxLoadsPerTick = 100;
    cfg.MaxUnloadsPerTick = 100;
    cfg.SaveOnUnload    = false;

    ChunkStreamer streamer;
    streamer.Init(&map, nullptr, cfg, 42);

    // Tick at origin — (10,0,10) has distSq=200 > 3²=9, so it's evicted.
    streamer.Tick({0, 0, 0});
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    streamer.Tick({0, 0, 0});

    REQUIRE_FALSE(map.HasChunk({10, 0, 10}));
    REQUIRE(streamer.TotalEvicted() >= 1);

    streamer.Shutdown();
}

TEST_CASE("ChunkStreamer: chunks within unload radius are kept", "[ChunkStreaming]") {
    ChunkMap map;
    map.CreateChunk({1, 0, 0});
    map.CreateChunk({0, 0, 1});

    ChunkStreamConfig cfg;
    cfg.LoadRadius      = 2;
    cfg.UnloadRadius    = 4;
    cfg.MaxLoadsPerTick = 0; // don't load new chunks for this test
    cfg.MaxUnloadsPerTick = 100;
    cfg.SaveOnUnload    = false;

    ChunkStreamer streamer;
    streamer.Init(&map, nullptr, cfg, 42);

    streamer.Tick({0, 0, 0});
    // (1,0,0) has distSq=1 ≤ 16, (0,0,1) has distSq=1 ≤ 16 — both within radius.
    REQUIRE(map.HasChunk({1, 0, 0}));
    REQUIRE(map.HasChunk({0, 0, 1}));

    streamer.Shutdown();
}

// ============================================================================
// ChunkStreamer — memory budget
// ============================================================================

TEST_CASE("ChunkStreamer: enforces MaxLoadedChunks budget", "[ChunkStreaming]") {
    ChunkMap map;
    // Pre-populate 20 chunks at various distances.
    for (int i = 0; i < 20; ++i)
        map.CreateChunk({i, 0, 0});
    REQUIRE(map.ChunkCount() == 20);

    ChunkStreamConfig cfg;
    cfg.LoadRadius      = 1;
    cfg.UnloadRadius    = 100; // don't unload by radius
    cfg.MaxLoadedChunks = 10;
    cfg.MaxLoadsPerTick = 0;
    cfg.MaxUnloadsPerTick = 100;
    cfg.SaveOnUnload    = false;

    ChunkStreamer streamer;
    streamer.Init(&map, nullptr, cfg, 42);

    streamer.Tick({0, 0, 0});

    // Budget should bring us to <= 10 chunks.
    REQUIRE(map.ChunkCount() <= 10);

    // The farthest chunks (large X) should be evicted first.
    REQUIRE(map.HasChunk({0, 0, 0}));   // closest
    REQUIRE_FALSE(map.HasChunk({19, 0, 0})); // farthest

    streamer.Shutdown();
}

// ============================================================================
// ChunkStreamer — background generation
// ============================================================================

TEST_CASE("ChunkStreamer: background thread generates terrain", "[ChunkStreaming]") {
    ChunkMap map;

    ChunkStreamConfig cfg;
    cfg.LoadRadius      = 1;
    cfg.UnloadRadius    = 3;
    cfg.MaxLoadsPerTick = 50;

    ChunkStreamer streamer;
    streamer.Init(&map, nullptr, cfg, 42);

    // Multiple ticks to queue + receive.
    for (int i = 0; i < 5; ++i) {
        streamer.Tick({0, 0, 0});
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Origin chunk should have been generated with terrain.
    Chunk* origin = map.GetChunk({0, 0, 0});
    REQUIRE(origin != nullptr);
    // The default generator fills Y=0 chunks with terrain (non-empty).
    REQUIRE_FALSE(origin->IsEmpty());

    REQUIRE(streamer.TotalGenerated() > 0);

    streamer.Shutdown();
}

TEST_CASE("ChunkStreamer: custom generator is used", "[ChunkStreaming]") {
    ChunkMap map;

    ChunkStreamConfig cfg;
    cfg.LoadRadius      = 0; // only origin
    cfg.UnloadRadius    = 3;
    cfg.MaxLoadsPerTick = 10;

    // Custom generator: fill with Metal.
    auto customGen = [](Chunk& chunk, uint32_t /*seed*/) {
        for (uint8_t x = 0; x < kChunkSize; ++x)
            for (uint8_t y = 0; y < kChunkSize; ++y)
                for (uint8_t z = 0; z < kChunkSize; ++z)
                    chunk.SetVoxel(x, y, z, static_cast<VoxelId>(VoxelType::Metal));
    };

    ChunkStreamer streamer;
    streamer.Init(&map, nullptr, cfg, 42, customGen);

    streamer.Tick({0, 0, 0});
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    streamer.Tick({0, 0, 0});

    Chunk* origin = map.GetChunk({0, 0, 0});
    REQUIRE(origin != nullptr);
    // Custom generator should have filled with Metal.
    REQUIRE(origin->GetVoxel(0, 0, 0) == static_cast<VoxelId>(VoxelType::Metal));
    REQUIRE(origin->GetVoxel(15, 15, 15) == static_cast<VoxelId>(VoxelType::Metal));

    streamer.Shutdown();
}

// ============================================================================
// ChunkStreamer — default generator correctness
// ============================================================================

TEST_CASE("ChunkStreamer: DefaultGenerator Y=0 produces terrain", "[ChunkStreaming]") {
    Chunk chunk({0, 0, 0});
    ChunkStreamer::DefaultGenerator(chunk, 42);

    // Surface chunks have non-air bottom layers.
    REQUIRE_FALSE(chunk.IsEmpty());
    REQUIRE(chunk.CountSolid() > 0);

    // Bottom layer should be Rock.
    REQUIRE(chunk.GetVoxel(0, 0, 0) == static_cast<VoxelId>(VoxelType::Rock));
}

TEST_CASE("ChunkStreamer: DefaultGenerator Y<0 is solid rock", "[ChunkStreaming]") {
    Chunk chunk({0, -1, 0});
    ChunkStreamer::DefaultGenerator(chunk, 42);

    // Underground chunks are solid rock.
    REQUIRE(chunk.CountSolid() == kChunkVolume);
    REQUIRE(chunk.GetVoxel(0, 0, 0) == static_cast<VoxelId>(VoxelType::Rock));
}

TEST_CASE("ChunkStreamer: DefaultGenerator Y>0 is empty air", "[ChunkStreaming]") {
    Chunk chunk({0, 1, 0});
    ChunkStreamer::DefaultGenerator(chunk, 42);

    // Above-surface chunks are empty.
    REQUIRE(chunk.IsEmpty());
}

TEST_CASE("ChunkStreamer: DefaultGenerator is seed-deterministic", "[ChunkStreaming]") {
    Chunk c1({3, 0, 5});
    Chunk c2({3, 0, 5});
    ChunkStreamer::DefaultGenerator(c1, 12345);
    ChunkStreamer::DefaultGenerator(c2, 12345);

    // Same seed + same coord → identical voxels.
    for (uint8_t x = 0; x < kChunkSize; ++x)
        for (uint8_t y = 0; y < kChunkSize; ++y)
            for (uint8_t z = 0; z < kChunkSize; ++z)
                REQUIRE(c1.GetVoxel(x, y, z) == c2.GetVoxel(x, y, z));
}

// ============================================================================
// ChunkStreamer — LOD computation
// ============================================================================

TEST_CASE("ChunkStreamer: ComputeLOD returns Full for close chunks", "[ChunkStreaming]") {
    // loadRadiusSq = 16 (radius=4), quarter = 4
    REQUIRE(ChunkStreamer::ComputeLOD(0, 16) == ChunkLOD::Full);
    REQUIRE(ChunkStreamer::ComputeLOD(3, 16) == ChunkLOD::Full);
    REQUIRE(ChunkStreamer::ComputeLOD(4, 16) == ChunkLOD::Full);
}

TEST_CASE("ChunkStreamer: ComputeLOD returns Simplified for mid-range", "[ChunkStreaming]") {
    REQUIRE(ChunkStreamer::ComputeLOD(5, 16) == ChunkLOD::Simplified);
    REQUIRE(ChunkStreamer::ComputeLOD(10, 16) == ChunkLOD::Simplified);
    REQUIRE(ChunkStreamer::ComputeLOD(16, 16) == ChunkLOD::Simplified);
}

TEST_CASE("ChunkStreamer: ComputeLOD returns None for distant chunks", "[ChunkStreaming]") {
    REQUIRE(ChunkStreamer::ComputeLOD(17, 16) == ChunkLOD::None);
    REQUIRE(ChunkStreamer::ComputeLOD(100, 16) == ChunkLOD::None);
}

// ============================================================================
// ChunkStreamer — DistanceSq
// ============================================================================

TEST_CASE("ChunkStreamer: DistanceSq at origin is zero", "[ChunkStreaming]") {
    REQUIRE(ChunkStreamer::DistanceSq({0, 0, 0}, {0, 0, 0}) == 0);
}

TEST_CASE("ChunkStreamer: DistanceSq is symmetric", "[ChunkStreaming]") {
    ChunkCoord a{3, 4, 5};
    ChunkCoord b{1, 2, 3};
    REQUIRE(ChunkStreamer::DistanceSq(a, b) == ChunkStreamer::DistanceSq(b, a));
}

TEST_CASE("ChunkStreamer: DistanceSq computes correctly", "[ChunkStreaming]") {
    // (3,0,4) to origin = 9 + 0 + 16 = 25
    REQUIRE(ChunkStreamer::DistanceSq({3, 0, 4}, {0, 0, 0}) == 25);
}

// ============================================================================
// ChunkStreamer — save-on-unload
// ============================================================================

TEST_CASE("ChunkStreamer: dirty chunk saved before eviction", "[ChunkStreaming]") {
    // Create a temp directory for saves.
    const std::string tmpDir = "/tmp/nf_chunk_test_" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::create_directories(tmpDir);

    ChunkMap map;
    // Place a dirty chunk far from viewer.
    Chunk* c = map.CreateChunk({20, 0, 20});
    c->SetVoxel(0, 0, 0, static_cast<VoxelId>(VoxelType::Stone));
    REQUIRE(c->IsDirty());

    ChunkStreamConfig cfg;
    cfg.LoadRadius      = 1;
    cfg.UnloadRadius    = 3;
    cfg.MaxLoadsPerTick = 0;
    cfg.MaxUnloadsPerTick = 100;
    cfg.SaveOnUnload    = true;
    cfg.SaveDirectory   = tmpDir;

    ChunkStreamer streamer;
    streamer.Init(&map, nullptr, cfg, 42);

    streamer.Tick({0, 0, 0});

    // Chunk should have been evicted.
    REQUIRE_FALSE(map.HasChunk({20, 0, 20}));

    // And a save file should exist.
    const std::string expectedFile = tmpDir + "/chunk_20_0_20.nfck";
    REQUIRE(std::filesystem::exists(expectedFile));

    // Clean up.
    std::filesystem::remove_all(tmpDir);
    streamer.Shutdown();
}

// ============================================================================
// ChunkStreamer — streaming as viewer moves
// ============================================================================

TEST_CASE("ChunkStreamer: chunks stream in as viewer moves", "[ChunkStreaming]") {
    ChunkMap map;

    ChunkStreamConfig cfg;
    cfg.LoadRadius      = 1;
    cfg.UnloadRadius    = 3;
    cfg.MaxLoadsPerTick = 50;
    cfg.MaxUnloadsPerTick = 50;
    cfg.SaveOnUnload    = false;

    ChunkStreamer streamer;
    streamer.Init(&map, nullptr, cfg, 42);

    // Tick at origin to load initial chunks.
    for (int i = 0; i < 5; ++i) {
        streamer.Tick({0, 0, 0});
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }
    REQUIRE(map.HasChunk({0, 0, 0}));

    // Move viewer to (5, 0, 0).
    for (int i = 0; i < 5; ++i) {
        streamer.Tick({5, 0, 0});
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }

    // New location should have loaded chunks.
    REQUIRE(map.HasChunk({5, 0, 0}));

    streamer.Shutdown();
}

// ============================================================================
// Orchestrator — Phase 8 integration
// ============================================================================

TEST_CASE("Orchestrator: Solo mode creates ChunkStreamer", "[ChunkStreaming]") {
    Orchestrator orch;
    orch.Init(nullptr);

    REQUIRE(orch.GetChunkStreamer() != nullptr);
    REQUIRE(orch.GetChunkStreamer()->IsInitialized());

    orch.Tick(1.f / 60.f);
    orch.Shutdown();
}
