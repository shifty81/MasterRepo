#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Game/Movement/PlayerMovement.h"
#include "Game/Voxel/ChunkMap.h"
#include "Game/Voxel/ChunkCoord.h"
#include "Game/Voxel/VoxelType.h"
#include <cmath>

using namespace NF::Game;
using Catch::Approx;

// ============================================================================
// ChunkMap — IsSolidAt
// ============================================================================

TEST_CASE("ChunkMap: IsSolidAt returns false for air", "[Movement]") {
    ChunkMap map;
    map.GetOrCreateChunk({0, 0, 0}); // all air by default
    REQUIRE_FALSE(map.IsSolidAt(0, 0, 0));
    REQUIRE_FALSE(map.IsSolidAt(15, 15, 15));
}

TEST_CASE("ChunkMap: IsSolidAt returns true for solid voxels", "[Movement]") {
    ChunkMap map;
    Chunk* c = map.GetOrCreateChunk({0, 0, 0});
    c->SetVoxel(5, 5, 5, static_cast<VoxelId>(VoxelType::Stone));

    REQUIRE(map.IsSolidAt(5, 5, 5));
    REQUIRE_FALSE(map.IsSolidAt(5, 5, 6)); // adjacent air
}

TEST_CASE("ChunkMap: IsSolidAt unloaded chunk default", "[Movement]") {
    ChunkMap map;
    // No chunks loaded — default is solid (safe collision fallback).
    REQUIRE(map.IsSolidAt(100, 100, 100, true));
    REQUIRE_FALSE(map.IsSolidAt(100, 100, 100, false));
}

TEST_CASE("ChunkMap: IsSolidAt negative coordinates", "[Movement]") {
    ChunkMap map;
    Chunk* c = map.GetOrCreateChunk({-1, 0, 0});
    c->SetVoxel(0, 0, 0, static_cast<VoxelId>(VoxelType::Rock));

    // Voxel at chunk(-1,0,0), local(0,0,0) → world(-32, 0, 0)
    REQUIRE(map.IsSolidAt(-32, 0, 0));
    REQUIRE_FALSE(map.IsSolidAt(-31, 0, 0));
}

// ============================================================================
// ChunkMap — RaycastVoxel
// ============================================================================

TEST_CASE("ChunkMap: RaycastVoxel hits a solid block", "[Movement]") {
    ChunkMap map;
    Chunk* c = map.GetOrCreateChunk({0, 0, 0});
    c->SetVoxel(5, 5, 5, static_cast<VoxelId>(VoxelType::Ore));

    // Cast ray from (5, 5, 0) in +Z direction.
    auto hit = map.RaycastVoxel(5.5f, 5.5f, 0.5f, 0.f, 0.f, 1.f, 32.f);
    REQUIRE(hit.hit);
    REQUIRE(hit.x == 5);
    REQUIRE(hit.y == 5);
    REQUIRE(hit.z == 5);
    REQUIRE(hit.id == static_cast<VoxelId>(VoxelType::Ore));
}

TEST_CASE("ChunkMap: RaycastVoxel misses when no solid", "[Movement]") {
    ChunkMap map;
    map.GetOrCreateChunk({0, 0, 0}); // all air

    auto hit = map.RaycastVoxel(0.5f, 0.5f, 0.5f, 1.f, 0.f, 0.f, 32.f);
    REQUIRE_FALSE(hit.hit);
}

TEST_CASE("ChunkMap: RaycastVoxel reports previous air voxel", "[Movement]") {
    ChunkMap map;
    Chunk* c = map.GetOrCreateChunk({0, 0, 0});
    c->SetVoxel(10, 5, 5, static_cast<VoxelId>(VoxelType::Stone));

    // Cast from (5, 5, 5) in +X direction.
    auto hit = map.RaycastVoxel(5.5f, 5.5f, 5.5f, 1.f, 0.f, 0.f, 32.f);
    REQUIRE(hit.hit);
    REQUIRE(hit.x == 10);
    REQUIRE(hit.prevX == 9); // voxel just before the hit
}

// ============================================================================
// PlayerMovement — Construction & Direction
// ============================================================================

TEST_CASE("PlayerMovement: default spawn position", "[Movement]") {
    PlayerMovement pm({0.f, 12.f, 0.f});
    REQUIRE(pm.GetPosition().X == Approx(0.f));
    REQUIRE(pm.GetPosition().Y == Approx(12.f));
    REQUIRE(pm.GetPosition().Z == Approx(0.f));
}

TEST_CASE("PlayerMovement: eye position is above feet", "[Movement]") {
    PlayerMovement pm({0.f, 10.f, 0.f});
    auto eye = pm.GetEyePosition();
    REQUIRE(eye.Y == Approx(10.f + PlayerMovement::kEyeHeight));
}

TEST_CASE("PlayerMovement: forward direction at yaw=0 is +Z", "[Movement]") {
    PlayerMovement pm;
    auto fwd = pm.GetForwardXZ();
    REQUIRE(fwd.X == Approx(0.f).margin(0.01f));
    REQUIRE(fwd.Z == Approx(1.f).margin(0.01f));
}

TEST_CASE("PlayerMovement: right direction at yaw=0 is +X", "[Movement]") {
    PlayerMovement pm;
    auto right = pm.GetRightXZ();
    REQUIRE(right.X == Approx(1.f).margin(0.01f));
    REQUIRE(right.Z == Approx(0.f).margin(0.01f));
}

TEST_CASE("PlayerMovement: mouse look changes yaw and pitch", "[Movement]") {
    PlayerMovement pm;
    REQUIRE(pm.GetYaw()   == Approx(0.f));
    REQUIRE(pm.GetPitch() == Approx(0.f));

    pm.ApplyMouseLook(100.f, 0.f);
    REQUIRE(pm.GetYaw() == Approx(100.f * PlayerMovement::kMouseSens));

    pm.ApplyMouseLook(0.f, 100.f);
    REQUIRE(pm.GetPitch() == Approx(-100.f * PlayerMovement::kMouseSens));
}

TEST_CASE("PlayerMovement: pitch is clamped", "[Movement]") {
    PlayerMovement pm;
    pm.ApplyMouseLook(0.f, -10000.f); // look way up
    REQUIRE(pm.GetPitch() <= PlayerMovement::kMaxPitch);

    pm.ApplyMouseLook(0.f, 20000.f); // look way down
    REQUIRE(pm.GetPitch() >= -PlayerMovement::kMaxPitch);
}

// ============================================================================
// PlayerMovement — Physics
// ============================================================================

TEST_CASE("PlayerMovement: gravity pulls player down in empty world", "[Movement]") {
    ChunkMap map;
    // No chunks loaded → IsSolidAt(..., false) returns false everywhere.

    PlayerMovement pm({0.f, 20.f, 0.f});
    pm.Update(0.016f, map);

    // Should have moved down due to gravity.
    REQUIRE(pm.GetPosition().Y < 20.f);
    REQUIRE_FALSE(pm.IsGrounded());
}

TEST_CASE("PlayerMovement: player lands on solid ground", "[Movement]") {
    ChunkMap map;
    Chunk* c = map.GetOrCreateChunk({0, 0, 0});
    // Fill y=0 layer with stone.
    for (uint8_t x = 0; x < kChunkSize; ++x)
        for (uint8_t z = 0; z < kChunkSize; ++z)
            c->SetVoxel(x, 0, z, static_cast<VoxelId>(VoxelType::Stone));

    // Start player just above the ground.
    PlayerMovement pm({5.f, 1.5f, 5.f});

    // Simulate several frames to let the player settle.
    for (int i = 0; i < 120; ++i)
        pm.Update(1.f / 60.f, map);

    REQUIRE(pm.IsGrounded());
    // Player feet should be resting at Y = 1.0 (top of voxel at y=0).
    REQUIRE(pm.GetPosition().Y == Approx(1.f).margin(0.1f));
}

TEST_CASE("PlayerMovement: WASD moves player horizontally", "[Movement]") {
    ChunkMap map;

    PlayerMovement pm({5.f, 20.f, 5.f});
    const float startZ = pm.GetPosition().Z;

    pm.SetMoveInput(1.f, 0.f, false, false); // forward = +Z at yaw=0
    pm.Update(0.5f, map);

    REQUIRE(pm.GetPosition().Z > startZ);
}

TEST_CASE("PlayerMovement: sprint moves faster than walk", "[Movement]") {
    ChunkMap map;

    PlayerMovement pmWalk({0.f, 20.f, 0.f});
    PlayerMovement pmSprint({0.f, 20.f, 0.f});

    pmWalk.SetMoveInput(1.f, 0.f, false, false);
    pmWalk.Update(0.5f, map);

    pmSprint.SetMoveInput(1.f, 0.f, false, true);
    pmSprint.Update(0.5f, map);

    REQUIRE(pmSprint.GetPosition().Z > pmWalk.GetPosition().Z);
}

TEST_CASE("PlayerMovement: jump gives upward velocity when grounded", "[Movement]") {
    ChunkMap map;
    Chunk* c = map.GetOrCreateChunk({0, 0, 0});
    for (uint8_t x = 0; x < kChunkSize; ++x)
        for (uint8_t z = 0; z < kChunkSize; ++z)
            c->SetVoxel(x, 0, z, static_cast<VoxelId>(VoxelType::Stone));

    PlayerMovement pm({5.f, 1.f, 5.f});

    // Settle on ground.
    for (int i = 0; i < 60; ++i)
        pm.Update(1.f / 60.f, map);
    REQUIRE(pm.IsGrounded());

    const float groundY = pm.GetPosition().Y;

    // Jump.
    pm.SetMoveInput(0.f, 0.f, true, false);
    pm.Update(1.f / 60.f, map);

    // Should be going up now.
    REQUIRE(pm.GetPosition().Y > groundY);
    REQUIRE(pm.GetVerticalVelocity() > 0.f);
}

TEST_CASE("PlayerMovement: collision prevents walking into solid wall", "[Movement]") {
    ChunkMap map;
    Chunk* c = map.GetOrCreateChunk({0, 0, 0});

    // Build a solid wall at z=10, from x=0..31, y=0..5.
    for (uint8_t x = 0; x < kChunkSize; ++x)
        for (uint8_t y = 0; y < 6; ++y)
            c->SetVoxel(x, y, 10, static_cast<VoxelId>(VoxelType::Stone));

    // Also add ground.
    for (uint8_t x = 0; x < kChunkSize; ++x)
        for (uint8_t z = 0; z < kChunkSize; ++z)
            c->SetVoxel(x, 0, z, static_cast<VoxelId>(VoxelType::Stone));

    PlayerMovement pm({5.f, 1.f, 8.f}); // start in front of wall

    // Walk forward (+Z) into the wall.
    for (int i = 0; i < 120; ++i) {
        pm.SetMoveInput(1.f, 0.f, false, false);
        pm.Update(1.f / 60.f, map);
    }

    // Should be stopped before z=10 (wall face minus player half-width).
    REQUIRE(pm.GetPosition().Z < 10.f);
}

TEST_CASE("PlayerMovement: SetPosition teleports player", "[Movement]") {
    PlayerMovement pm({0.f, 0.f, 0.f});
    pm.SetPosition({100.f, 50.f, 200.f});
    REQUIRE(pm.GetPosition().X == Approx(100.f));
    REQUIRE(pm.GetPosition().Y == Approx(50.f));
    REQUIRE(pm.GetPosition().Z == Approx(200.f));
}

TEST_CASE("PlayerMovement: view direction incorporates pitch", "[Movement]") {
    PlayerMovement pm;
    pm.ApplyMouseLook(0.f, -200.f); // look up
    auto dir = pm.GetViewDirection();
    REQUIRE(dir.Y > 0.f); // looking up
}
