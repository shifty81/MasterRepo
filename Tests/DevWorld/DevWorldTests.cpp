#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Game/World/DevWorldConfig.h"
#include "Game/World/WorldSaveLoad.h"
#include "Game/World/WorldDebugOverlay.h"
#include "Game/World/GameWorld.h"
#include <cstdint>
#include <filesystem>
#include <vector>

using namespace NF;
using namespace NF::Game;

namespace {

/// @brief Locate Content/Definitions/DevWorld.json from the CWD.
/// CTest may run from build/bin/, build/, or the repo root.
std::string FindContentRoot()
{
    DevWorldConfig probe;
    if (probe.LoadFromFile("Content/Definitions/DevWorld.json"))
        return "Content";
    if (probe.LoadFromFile("../Content/Definitions/DevWorld.json"))
        return "../Content";
    if (probe.LoadFromFile("../../Content/Definitions/DevWorld.json"))
        return "../../Content";
    return {};
}

/// @brief Return a cross-platform path in the temp directory.
std::string TempPath(const std::string& filename) {
    return (std::filesystem::temp_directory_path() / filename).string();
}

} // anonymous namespace

// =============================================================================
// DevWorldConfig
// =============================================================================

TEST_CASE("DevWorldConfig::Defaults returns valid config", "[DevWorld]") {
    auto cfg = DevWorldConfig::Defaults();
    REQUIRE(cfg.IsValid());
    REQUIRE(cfg.WorldId() == "DevWorld");
    REQUIRE(cfg.DisplayName() == "Development Sandbox");
    REQUIRE(cfg.Seed() == 42);
}

TEST_CASE("DevWorldConfig::Defaults has correct spawn point", "[DevWorld]") {
    auto cfg = DevWorldConfig::Defaults();
    const auto& sp = cfg.GetSpawnPoint();
    REQUIRE_THAT(sp.Position.X, Catch::Matchers::WithinAbs(128.0, 0.01));
    REQUIRE_THAT(sp.Position.Y, Catch::Matchers::WithinAbs(32.0, 0.01));
    REQUIRE_THAT(sp.Position.Z, Catch::Matchers::WithinAbs(128.0, 0.01));
}

TEST_CASE("DevWorldConfig::Defaults has correct camera config", "[DevWorld]") {
    auto cfg = DevWorldConfig::Defaults();
    const auto& cam = cfg.GetCameraConfig();
    REQUIRE_THAT(cam.FOV, Catch::Matchers::WithinAbs(75.0, 0.01));
    REQUIRE_THAT(cam.NearClip, Catch::Matchers::WithinAbs(0.1, 0.001));
    REQUIRE_THAT(cam.FarClip, Catch::Matchers::WithinAbs(1000.0, 0.01));
}

TEST_CASE("DevWorldConfig loads from DevWorld.json", "[DevWorld]") {
    auto contentRoot = FindContentRoot();
    REQUIRE(!contentRoot.empty());
    DevWorldConfig cfg;
    bool loaded = cfg.LoadFromFile(contentRoot + "/Definitions/DevWorld.json");
    REQUIRE(loaded);
    REQUIRE(cfg.IsValid());
    REQUIRE(cfg.WorldId() == "DevWorld");
    REQUIRE(cfg.Seed() == 42);
}

TEST_CASE("DevWorldConfig fails gracefully on missing file", "[DevWorld]") {
    DevWorldConfig cfg;
    bool loaded = cfg.LoadFromFile("nonexistent/path.json");
    REQUIRE_FALSE(loaded);
    REQUIRE_FALSE(cfg.IsValid());
}

// =============================================================================
// WorldSaveLoad — in-memory round-trip
// =============================================================================

TEST_CASE("WorldSaveLoad round-trips through memory", "[WorldSave]") {
    const uint32_t seed = 42;
    const std::vector<uint32_t> ids = {1, 2, 3, 5, 8};

    WorldSaveLoad saver;
    auto bytes = saver.SaveToMemory(seed, ids);
    REQUIRE(!bytes.empty());

    WorldSaveLoad loader;
    bool ok = loader.LoadFromMemory(bytes.data(), bytes.size());
    REQUIRE(ok);
    REQUIRE(loader.GetHeader().Seed == seed);
    REQUIRE(loader.GetHeader().EntityCount == ids.size());
    REQUIRE(loader.GetEntityIds() == ids);
}

TEST_CASE("WorldSaveLoad validates magic number", "[WorldSave]") {
    std::vector<std::byte> bad(16, std::byte{0xFF});
    WorldSaveLoad loader;
    REQUIRE_FALSE(loader.LoadFromMemory(bad.data(), bad.size()));
}

TEST_CASE("WorldSaveLoad rejects truncated data", "[WorldSave]") {
    WorldSaveLoad loader;
    std::byte tiny[2]{};
    REQUIRE_FALSE(loader.LoadFromMemory(tiny, sizeof(tiny)));
}

TEST_CASE("WorldSaveLoad round-trips with zero entities", "[WorldSave]") {
    WorldSaveLoad saver;
    auto bytes = saver.SaveToMemory(99, {});
    REQUIRE(!bytes.empty());

    WorldSaveLoad loader;
    REQUIRE(loader.LoadFromMemory(bytes.data(), bytes.size()));
    REQUIRE(loader.GetHeader().Seed == 99);
    REQUIRE(loader.GetHeader().EntityCount == 0);
    REQUIRE(loader.GetEntityIds().empty());
}

TEST_CASE("WorldSaveLoad file round-trip", "[WorldSave]") {
    const std::string path = TempPath("nf_test_save.nfsv");
    const uint32_t seed = 7;
    const std::vector<uint32_t> ids = {10, 20, 30};

    WorldSaveLoad saver;
    REQUIRE(saver.SaveToFile(path, seed, ids));

    WorldSaveLoad loader;
    REQUIRE(loader.LoadFromFile(path));
    REQUIRE(loader.GetHeader().Seed == seed);
    REQUIRE(loader.GetEntityIds() == ids);

    // Clean up
    std::remove(path.c_str());
}

// =============================================================================
// WorldDebugOverlay
// =============================================================================

TEST_CASE("WorldDebugOverlay generates lines when enabled", "[DebugOverlay]") {
    auto cfg = DevWorldConfig::Defaults();
    WorldDebugOverlay overlay;
    overlay.SetEnabled(true);
    overlay.Update(cfg, nullptr, NullEntity);

    const auto& lines = overlay.GetLines();
    REQUIRE(!lines.empty());
    REQUIRE(lines.front() == "=== World Debug Overlay ===");
    REQUIRE(lines.back() == "===========================");
}

TEST_CASE("WorldDebugOverlay produces no lines when disabled", "[DebugOverlay]") {
    auto cfg = DevWorldConfig::Defaults();
    WorldDebugOverlay overlay;
    overlay.SetEnabled(false);
    overlay.Update(cfg, nullptr, NullEntity);
    REQUIRE(overlay.GetLines().empty());
}

TEST_CASE("WorldDebugOverlay includes seed", "[DebugOverlay]") {
    auto cfg = DevWorldConfig::Defaults();
    WorldDebugOverlay overlay;
    overlay.Update(cfg, nullptr, NullEntity);

    bool foundSeed = false;
    for (const auto& line : overlay.GetLines()) {
        if (line.find("Seed") != std::string::npos &&
            line.find("42") != std::string::npos) {
            foundSeed = true;
            break;
        }
    }
    REQUIRE(foundSeed);
}

// =============================================================================
// GameWorld integration
// =============================================================================

TEST_CASE("GameWorld initializes with defaults when config missing", "[GameWorld]") {
    GameWorld gw;
    REQUIRE(gw.Initialize("nonexistent_content"));
    REQUIRE(gw.IsReady());
    REQUIRE(gw.GetConfig().IsValid());
    REQUIRE(gw.GetPlayerEntity() != NullEntity);
    gw.Shutdown();
    REQUIRE_FALSE(gw.IsReady());
}

TEST_CASE("GameWorld initializes with DevWorld.json", "[GameWorld]") {
    auto contentRoot = FindContentRoot();
    REQUIRE(!contentRoot.empty());
    GameWorld gw;
    REQUIRE(gw.Initialize(contentRoot));
    REQUIRE(gw.IsReady());
    REQUIRE(gw.GetConfig().WorldId() == "DevWorld");
    REQUIRE(gw.GetConfig().Seed() == 42);
    gw.Shutdown();
}

TEST_CASE("GameWorld spawn point matches config", "[GameWorld]") {
    auto contentRoot = FindContentRoot();
    REQUIRE(!contentRoot.empty());
    GameWorld gw;
    gw.Initialize(contentRoot);
    const auto& sp = gw.GetSpawnPoint();
    REQUIRE_THAT(sp.Position.X, Catch::Matchers::WithinAbs(128.0, 0.01));
    REQUIRE_THAT(sp.Position.Y, Catch::Matchers::WithinAbs(32.0, 0.01));
    REQUIRE_THAT(sp.Position.Z, Catch::Matchers::WithinAbs(128.0, 0.01));
    gw.Shutdown();
}

TEST_CASE("GameWorld save and load round-trip", "[GameWorld]") {
    auto contentRoot = FindContentRoot();
    REQUIRE(!contentRoot.empty());
    const std::string savePath = TempPath("nf_gameworld_save.nfsv");
    GameWorld gw;
    gw.Initialize(contentRoot);

    REQUIRE(gw.SaveWorld(savePath));
    REQUIRE(gw.LoadWorld(savePath));

    gw.Shutdown();
    std::remove(savePath.c_str());
}

TEST_CASE("GameWorld debug overlay works", "[GameWorld]") {
    auto contentRoot = FindContentRoot();
    REQUIRE(!contentRoot.empty());
    GameWorld gw;
    gw.Initialize(contentRoot);
    gw.LogDebugOverlay();
    REQUIRE(!gw.GetDebugOverlay().GetLines().empty());
    gw.Shutdown();
}

TEST_CASE("GameWorld tick does not crash", "[GameWorld]") {
    auto contentRoot = FindContentRoot();
    REQUIRE(!contentRoot.empty());
    GameWorld gw;
    gw.Initialize(contentRoot);
    gw.Tick(1.0f / 60.0f);
    gw.Tick(1.0f / 60.0f);
    gw.Tick(1.0f / 60.0f);
    gw.Shutdown();
}

TEST_CASE("GameWorld cannot save when not initialized", "[GameWorld]") {
    GameWorld gw;
    REQUIRE_FALSE(gw.SaveWorld(TempPath("should_not_exist.nfsv")));
}
