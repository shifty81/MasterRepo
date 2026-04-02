#include <catch2/catch_test_macros.hpp>
#include "Game/Voxel/VoxelType.h"
#include "Game/Voxel/ChunkCoord.h"
#include "Game/Voxel/Chunk.h"
#include "Game/Voxel/ChunkMap.h"
#include "Game/Voxel/VoxelEditApi.h"
#include "Game/Voxel/VoxelSerializer.h"
#include "Game/Voxel/VoxelDebugOverlay.h"

using namespace NF::Game;

// =============================================================================
// VoxelType
// =============================================================================

TEST_CASE("VoxelTypeInfo: Air is id 0 and not solid", "[Voxel]") {
    const auto& info = GetVoxelTypeInfo(VoxelType::Air);
    REQUIRE(info.id      == 0);
    REQUIRE(!info.isSolid);
    REQUIRE(!info.isMineable);
    REQUIRE(info.hardness == 0);
}

TEST_CASE("VoxelTypeInfo: Stone is solid and minable", "[Voxel]") {
    const auto& info = GetVoxelTypeInfo(VoxelType::Stone);
    REQUIRE(info.isSolid);
    REQUIRE(info.isMineable);
    REQUIRE(info.hardness >= 1);
    REQUIRE(std::string_view(info.name) == "Stone");
}

TEST_CASE("VoxelTypeInfo: Ore has higher hardness than Dirt", "[Voxel]") {
    REQUIRE(GetVoxelTypeInfo(VoxelType::Ore).hardness >
            GetVoxelTypeInfo(VoxelType::Dirt).hardness);
}

TEST_CASE("VoxelTypeInfo: unknown id falls back to Air", "[Voxel]") {
    const auto& info = GetVoxelTypeInfo(static_cast<VoxelId>(255));
    REQUIRE(info.id == 0);
}

// =============================================================================
// ChunkCoord
// =============================================================================

TEST_CASE("WorldToChunk: positive coords", "[ChunkCoord]") {
    auto cc = WorldToChunk(0, 0, 0);
    REQUIRE(cc.X == 0); REQUIRE(cc.Y == 0); REQUIRE(cc.Z == 0);

    cc = WorldToChunk(31, 31, 31);
    REQUIRE(cc.X == 0); REQUIRE(cc.Y == 0); REQUIRE(cc.Z == 0);

    cc = WorldToChunk(32, 0, 0);
    REQUIRE(cc.X == 1); REQUIRE(cc.Y == 0); REQUIRE(cc.Z == 0);

    cc = WorldToChunk(63, 63, 63);
    REQUIRE(cc.X == 1); REQUIRE(cc.Y == 1); REQUIRE(cc.Z == 1);
}

TEST_CASE("WorldToChunk: negative coords floor-divide correctly", "[ChunkCoord]") {
    auto cc = WorldToChunk(-1, -1, -1);
    REQUIRE(cc.X == -1); REQUIRE(cc.Y == -1); REQUIRE(cc.Z == -1);

    cc = WorldToChunk(-32, -32, -32);
    REQUIRE(cc.X == -1); REQUIRE(cc.Y == -1); REQUIRE(cc.Z == -1);

    cc = WorldToChunk(-33, 0, 0);
    REQUIRE(cc.X == -2);
}

TEST_CASE("WorldToLocal: within chunk bounds", "[ChunkCoord]") {
    auto lc = WorldToLocal(0, 0, 0);
    REQUIRE(lc.X == 0); REQUIRE(lc.Y == 0); REQUIRE(lc.Z == 0);

    lc = WorldToLocal(31, 31, 31);
    REQUIRE(lc.X == 31); REQUIRE(lc.Y == 31); REQUIRE(lc.Z == 31);

    lc = WorldToLocal(32, 0, 0);
    REQUIRE(lc.X == 0); REQUIRE(lc.Y == 0); REQUIRE(lc.Z == 0);
}

TEST_CASE("WorldToLocal: negative coords wrap correctly", "[ChunkCoord]") {
    auto lc = WorldToLocal(-1, -1, -1);
    REQUIRE(lc.X == 31); REQUIRE(lc.Y == 31); REQUIRE(lc.Z == 31);
}

TEST_CASE("LocalToIndex: x varies fastest", "[ChunkCoord]") {
    REQUIRE(LocalToIndex(0, 0, 0) == 0);
    REQUIRE(LocalToIndex(1, 0, 0) == 1);
    REQUIRE(LocalToIndex(0, 1, 0) == kChunkSize);
    REQUIRE(LocalToIndex(0, 0, 1) == kChunkSize * kChunkSize);
}

TEST_CASE("ChunkCoord equality", "[ChunkCoord]") {
    REQUIRE(ChunkCoord{1, 2, 3} == ChunkCoord{1, 2, 3});
    REQUIRE(ChunkCoord{1, 2, 3} != ChunkCoord{1, 2, 4});
}

TEST_CASE("kChunkVolume is 32768", "[ChunkCoord]") {
    REQUIRE(kChunkVolume == 32768);
}

// =============================================================================
// Chunk
// =============================================================================

TEST_CASE("Chunk: default state is all Air and not dirty", "[Chunk]") {
    Chunk c({0, 0, 0});
    REQUIRE(c.IsEmpty());
    REQUIRE(!c.IsDirty());
    REQUIRE(c.CountSolid() == 0);
}

TEST_CASE("Chunk: SetVoxel marks dirty", "[Chunk]") {
    Chunk c({0, 0, 0});
    REQUIRE(c.SetVoxel(1, 2, 3, static_cast<VoxelId>(VoxelType::Stone)));
    REQUIRE(c.IsDirty());
    REQUIRE(c.GetVoxel(1, 2, 3) == static_cast<VoxelId>(VoxelType::Stone));
}

TEST_CASE("Chunk: ClearDirty resets flag", "[Chunk]") {
    Chunk c({0, 0, 0});
    c.SetVoxel(0, 0, 0, static_cast<VoxelId>(VoxelType::Stone));
    REQUIRE(c.IsDirty());
    c.ClearDirty();
    REQUIRE(!c.IsDirty());
}

TEST_CASE("Chunk: CountSolid counts non-air voxels", "[Chunk]") {
    Chunk c({0, 0, 0});
    c.SetVoxel(0, 0, 0, static_cast<VoxelId>(VoxelType::Stone));
    c.SetVoxel(1, 0, 0, static_cast<VoxelId>(VoxelType::Ore));
    REQUIRE(c.CountSolid() == 2);
}

TEST_CASE("Chunk: SetVoxel out of bounds returns false", "[Chunk]") {
    Chunk c({0, 0, 0});
    REQUIRE(!c.SetVoxel(32, 0, 0, static_cast<VoxelId>(VoxelType::Stone)));
    REQUIRE(!c.SetVoxel(0, 32, 0, static_cast<VoxelId>(VoxelType::Stone)));
    REQUIRE(!c.SetVoxel(0, 0, 32, static_cast<VoxelId>(VoxelType::Stone)));
    REQUIRE(c.IsEmpty());
}

TEST_CASE("Chunk: GetCoord returns construction coord", "[Chunk]") {
    ChunkCoord cc{3, -1, 7};
    Chunk c(cc);
    REQUIRE(c.GetCoord() == cc);
}

// =============================================================================
// ChunkMap
// =============================================================================

TEST_CASE("ChunkMap: initially empty", "[ChunkMap]") {
    ChunkMap map;
    REQUIRE(map.ChunkCount() == 0);
    REQUIRE(!map.HasChunk({0, 0, 0}));
    REQUIRE(map.GetChunk({0, 0, 0}) == nullptr);
}

TEST_CASE("ChunkMap: CreateChunk inserts and returns a chunk", "[ChunkMap]") {
    ChunkMap map;
    Chunk* c = map.CreateChunk({1, 2, 3});
    REQUIRE(c != nullptr);
    REQUIRE(c->GetCoord() == ChunkCoord{1, 2, 3});
    REQUIRE(map.HasChunk({1, 2, 3}));
    REQUIRE(map.ChunkCount() == 1);
}

TEST_CASE("ChunkMap: GetOrCreateChunk creates on first access", "[ChunkMap]") {
    ChunkMap map;
    Chunk* c = map.GetOrCreateChunk({5, 5, 5});
    REQUIRE(c != nullptr);
    REQUIRE(map.HasChunk({5, 5, 5}));
    Chunk* c2 = map.GetOrCreateChunk({5, 5, 5});
    REQUIRE(c == c2); // same pointer
}

TEST_CASE("ChunkMap: UnloadChunk removes it", "[ChunkMap]") {
    ChunkMap map;
    map.CreateChunk({0, 0, 0});
    map.UnloadChunk({0, 0, 0});
    REQUIRE(!map.HasChunk({0, 0, 0}));
    REQUIRE(map.ChunkCount() == 0);
}

TEST_CASE("ChunkMap: Clear removes all chunks", "[ChunkMap]") {
    ChunkMap map;
    map.CreateChunk({0, 0, 0});
    map.CreateChunk({1, 0, 0});
    map.Clear();
    REQUIRE(map.ChunkCount() == 0);
}

TEST_CASE("ChunkMap: GetDirtyChunks returns only dirty chunks", "[ChunkMap]") {
    ChunkMap map;
    Chunk* c0 = map.CreateChunk({0, 0, 0});
    Chunk* c1 = map.CreateChunk({1, 0, 0});
    c1->SetVoxel(0, 0, 0, static_cast<VoxelId>(VoxelType::Stone));

    auto dirty = map.GetDirtyChunks();
    REQUIRE(dirty.size() == 1);
    REQUIRE(dirty[0] == c1);
    (void)c0;
}

// =============================================================================
// VoxelEditApi
// =============================================================================

TEST_CASE("VoxelEditApi: GetVoxel returns Air for unloaded chunk", "[VoxelEdit]") {
    ChunkMap map;
    VoxelEditApi api(map);
    REQUIRE(api.GetVoxel(0, 0, 0) == static_cast<VoxelId>(VoxelType::Air));
}

TEST_CASE("VoxelEditApi: SetVoxel creates chunk and writes voxel", "[VoxelEdit]") {
    ChunkMap map;
    VoxelEditApi api(map);
    api.SetVoxel(5, 10, 15, VoxelType::Stone);
    REQUIRE(api.GetVoxel(5, 10, 15) == static_cast<VoxelId>(VoxelType::Stone));
    REQUIRE(map.HasChunk(WorldToChunk(5, 10, 15)));
}

TEST_CASE("VoxelEditApi: SetVoxel across chunk boundary", "[VoxelEdit]") {
    ChunkMap map;
    VoxelEditApi api(map);
    api.SetVoxel(31, 0, 0, VoxelType::Stone); // chunk (0,0,0)
    api.SetVoxel(32, 0, 0, VoxelType::Ore);   // chunk (1,0,0)
    REQUIRE(api.GetVoxel(31, 0, 0) == static_cast<VoxelId>(VoxelType::Stone));
    REQUIRE(api.GetVoxel(32, 0, 0) == static_cast<VoxelId>(VoxelType::Ore));
    REQUIRE(map.ChunkCount() == 2);
}

TEST_CASE("VoxelEditApi: Mine instantly removes mineable voxel", "[VoxelEdit]") {
    ChunkMap map;
    VoxelEditApi api(map);
    api.SetVoxel(0, 0, 0, VoxelType::Stone);

    auto report = api.Mine(0, 0, 0);
    REQUIRE(report.result == MineResult::Success);
    REQUIRE(report.extractedType == static_cast<VoxelId>(VoxelType::Stone));
    REQUIRE(api.GetVoxel(0, 0, 0) == static_cast<VoxelId>(VoxelType::Air));
}

TEST_CASE("VoxelEditApi: Mine fails on Air", "[VoxelEdit]") {
    ChunkMap map;
    VoxelEditApi api(map);
    auto report = api.Mine(0, 0, 0);
    REQUIRE(report.result == MineResult::NotMineable);
}

TEST_CASE("VoxelEditApi: Mine with durability reports remaining", "[VoxelEdit]") {
    ChunkMap map;
    VoxelEditApi api(map);
    api.SetVoxel(0, 0, 0, VoxelType::Stone); // hardness = 2

    // Pass durability larger than hardness — voxel takes damage but survives.
    auto report = api.Mine(0, 0, 0, 10);
    REQUIRE(report.result == MineResult::DurabilityLeft);
    REQUIRE(report.remainingDurability < 10);
    REQUIRE(api.GetVoxel(0, 0, 0) == static_cast<VoxelId>(VoxelType::Stone));
}

TEST_CASE("VoxelEditApi: Damage destroys voxel when durability <= hardness", "[VoxelEdit]") {
    ChunkMap map;
    VoxelEditApi api(map);
    api.SetVoxel(0, 0, 0, VoxelType::Dirt); // hardness = 1
    uint8_t remaining = api.Damage(0, 0, 0, 1);
    REQUIRE(remaining == 0);
    REQUIRE(api.GetVoxel(0, 0, 0) == static_cast<VoxelId>(VoxelType::Air));
}

TEST_CASE("VoxelEditApi: Repair places a voxel back", "[VoxelEdit]") {
    ChunkMap map;
    VoxelEditApi api(map);
    api.SetVoxel(0, 0, 0, VoxelType::Stone);
    api.Mine(0, 0, 0); // remove it
    REQUIRE(api.GetVoxel(0, 0, 0) == static_cast<VoxelId>(VoxelType::Air));

    api.Repair(0, 0, 0, static_cast<VoxelId>(VoxelType::Metal));
    REQUIRE(api.GetVoxel(0, 0, 0) == static_cast<VoxelId>(VoxelType::Metal));
}

TEST_CASE("VoxelEditApi: negative world coords work correctly", "[VoxelEdit]") {
    ChunkMap map;
    VoxelEditApi api(map);
    api.SetVoxel(-1, -1, -1, VoxelType::Ice);
    REQUIRE(api.GetVoxel(-1, -1, -1) == static_cast<VoxelId>(VoxelType::Ice));
}

// =============================================================================
// VoxelSerializer
// =============================================================================

TEST_CASE("VoxelSerializer: single chunk round-trip", "[VoxelSerializer]") {
    ChunkCoord cc{2, 0, -1};
    Chunk original(cc);
    original.SetVoxel(5,  5,  5,  static_cast<VoxelId>(VoxelType::Stone));
    original.SetVoxel(10, 10, 10, static_cast<VoxelId>(VoxelType::Ore));

    auto bytes = VoxelSerializer::SerializeChunk(original);
    REQUIRE(!bytes.empty());

    Chunk loaded(cc);
    REQUIRE(VoxelSerializer::DeserializeChunk(bytes.data(), bytes.size(), loaded));
    REQUIRE(loaded.GetVoxel(5,  5,  5)  == static_cast<VoxelId>(VoxelType::Stone));
    REQUIRE(loaded.GetVoxel(10, 10, 10) == static_cast<VoxelId>(VoxelType::Ore));
    REQUIRE(!loaded.IsDirty()); // ClearDirty called during load
}

TEST_CASE("VoxelSerializer: chunk map round-trip in memory", "[VoxelSerializer]") {
    ChunkMap original;
    VoxelEditApi api(original);
    api.SetVoxel(0,  0,  0,  VoxelType::Stone);
    api.SetVoxel(32, 0,  0,  VoxelType::Ore);
    api.SetVoxel(64, 64, 64, VoxelType::Metal);

    auto bytes = VoxelSerializer::SerializeMap(original);
    REQUIRE(!bytes.empty());

    ChunkMap loaded;
    REQUIRE(VoxelSerializer::DeserializeMap(loaded, bytes.data(), bytes.size()));
    REQUIRE(loaded.ChunkCount() == original.ChunkCount());

    VoxelEditApi apiL(loaded);
    REQUIRE(apiL.GetVoxel(0,  0,  0)  == static_cast<VoxelId>(VoxelType::Stone));
    REQUIRE(apiL.GetVoxel(32, 0,  0)  == static_cast<VoxelId>(VoxelType::Ore));
    REQUIRE(apiL.GetVoxel(64, 64, 64) == static_cast<VoxelId>(VoxelType::Metal));
}

TEST_CASE("VoxelSerializer: empty map serializes to minimal bytes", "[VoxelSerializer]") {
    ChunkMap empty;
    auto bytes = VoxelSerializer::SerializeMap(empty);
    // Should contain at least the 4-byte count header.
    REQUIRE(bytes.size() >= sizeof(uint32_t));

    ChunkMap loaded;
    REQUIRE(VoxelSerializer::DeserializeMap(loaded, bytes.data(), bytes.size()));
    REQUIRE(loaded.ChunkCount() == 0);
}

TEST_CASE("VoxelSerializer: corrupt magic rejected", "[VoxelSerializer]") {
    ChunkCoord cc{0, 0, 0};
    Chunk original(cc);
    original.SetVoxel(0, 0, 0, static_cast<VoxelId>(VoxelType::Stone));

    auto bytes = VoxelSerializer::SerializeChunk(original);
    // Corrupt the magic.
    bytes[0] = std::byte{0xDE};
    bytes[1] = std::byte{0xAD};

    Chunk bad(cc);
    REQUIRE(!VoxelSerializer::DeserializeChunk(bytes.data(), bytes.size(), bad));
}

// =============================================================================
// VoxelDebugOverlay
// =============================================================================

TEST_CASE("VoxelDebugOverlay: ValidateMap returns true on clean data", "[VoxelDebug]") {
    ChunkMap map;
    VoxelEditApi api(map);
    api.SetVoxel(0, 0, 0, VoxelType::Stone);
    api.SetVoxel(1, 0, 0, VoxelType::Ore);

    REQUIRE(VoxelDebugOverlay::ValidateMap(map));
}

TEST_CASE("VoxelDebugOverlay: ValidateMap passes for empty map", "[VoxelDebug]") {
    ChunkMap map;
    REQUIRE(VoxelDebugOverlay::ValidateMap(map));
}

TEST_CASE("VoxelDebugOverlay: BuildStatsString is non-empty", "[VoxelDebug]") {
    ChunkMap map;
    VoxelEditApi api(map);
    api.SetVoxel(0, 0, 0, VoxelType::Stone);

    const std::string stats = VoxelDebugOverlay::BuildStatsString(map);
    REQUIRE(!stats.empty());
    REQUIRE(stats.find("Chunks") != std::string::npos);
}
