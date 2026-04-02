#include <catch2/catch_test_macros.hpp>
#include "Core/Config/ProjectManifest.h"
#include "Game/App/Bootstrap.h"
#include "Game/App/ProjectContext.h"
#include "Game/App/Session.h"

// ============================================================================
// Startup Smoke Tests — Phase 0 Bootstrap
// ============================================================================

TEST_CASE("ProjectManifest loads novaforge.project.json", "[bootstrap][manifest]")
{
    // Test executable may run from build/bin/, build/, or repo root
    NF::ProjectManifest manifest;
    bool loaded = manifest.LoadFromFile("Config/novaforge.project.json");
    if (!loaded)
        loaded = manifest.LoadFromFile("../Config/novaforge.project.json");
    if (!loaded)
        loaded = manifest.LoadFromFile("../../Config/novaforge.project.json");

    REQUIRE(loaded);
    REQUIRE(manifest.IsValid());
    CHECK(manifest.ProjectName    == "NovaForge");
    CHECK(manifest.ProjectVersion == "0.1.0-reset");
    CHECK(manifest.ProjectType    == "standalone-game-repo");
    CHECK(manifest.DefaultWorld   == "DevWorld");
    CHECK(manifest.ContentRoot    == "Content");
    CHECK(manifest.ConfigRoot     == "Config");
    CHECK(manifest.SaveRoot       == "Saved");
    CHECK(manifest.LogRoot        == "Saved/Logs");
    CHECK(manifest.VoxelAuthoritative      == true);
    CHECK(manifest.EditorShipsWithGame     == false);
    CHECK(manifest.AllowSuiteFeaturesInRepo == false);
    CHECK(manifest.Phases.size()  == 4);
}

TEST_CASE("ProjectManifest fails gracefully on missing file", "[bootstrap][manifest]")
{
    NF::ProjectManifest manifest;
    bool loaded = manifest.LoadFromFile("nonexistent/path.json");

    REQUIRE_FALSE(loaded);
    // Defaults should still be valid
    CHECK(manifest.IsValid());
}

TEST_CASE("Bootstrap runs successfully with valid RepoRoot", "[bootstrap]")
{
    // Determine repo root relative to CWD (may be build/bin/, build/, or repo root)
    NF::ProjectManifest probe;
    std::string repoRoot = ".";
    if (!probe.LoadFromFile("Config/novaforge.project.json"))
    {
        repoRoot = "..";
        if (!probe.LoadFromFile("../Config/novaforge.project.json"))
            repoRoot = "../..";
    }

    NF::Game::Bootstrap bootstrap;
    NF::Game::BootstrapConfig cfg;
    cfg.RepoRoot = repoRoot;

    auto result = bootstrap.Run(cfg);

    REQUIRE(result.Success);
    CHECK(result.Message == "Bootstrap complete");
    CHECK(bootstrap.IsRunning());
    CHECK(bootstrap.GetProjectContext() != nullptr);
    CHECK(bootstrap.GetProjectContext()->IsValid());
    CHECK(bootstrap.GetSession() != nullptr);
    CHECK(bootstrap.GetSession()->IsConnected());

    bootstrap.Shutdown();
    CHECK_FALSE(bootstrap.IsRunning());
}

TEST_CASE("Bootstrap fails with empty RepoRoot", "[bootstrap]")
{
    NF::Game::Bootstrap bootstrap;
    NF::Game::BootstrapConfig cfg;
    cfg.RepoRoot = "";

    auto result = bootstrap.Run(cfg);

    REQUIRE_FALSE(result.Success);
    CHECK_FALSE(bootstrap.IsRunning());
}

TEST_CASE("ProjectContext resolves paths from manifest", "[bootstrap][context]")
{
    NF::Game::ProjectContextConfig cfg;
    cfg.repoRoot    = "/test/root";
    cfg.contentRoot = "Content";
    cfg.displayName = "NovaForge";
    cfg.version     = "0.1.0-reset";

    NF::Game::ProjectContext ctx(cfg);

    REQUIRE(ctx.IsValid());
    CHECK(ctx.DisplayName() == "NovaForge");
    CHECK(ctx.Version()     == "0.1.0-reset");
    CHECK(ctx.RepoRoot()    == "/test/root");
    CHECK(ctx.ContentRoot() == "/test/root/Content");
}

TEST_CASE("Session lifecycle transitions work correctly", "[bootstrap][session]")
{
    NF::Game::Session session;

    CHECK(session.GetState() == NF::Game::SessionState::Disconnected);
    CHECK_FALSE(session.IsConnected());

    session.OnConnecting();
    CHECK(session.GetState() == NF::Game::SessionState::Connecting);

    session.OnConnected("test-token");
    CHECK(session.IsConnected());
    CHECK(session.GetToken() == "test-token");

    session.OnDisconnecting();
    CHECK(session.GetState() == NF::Game::SessionState::Disconnecting);

    session.OnDisconnected();
    CHECK(session.GetState() == NF::Game::SessionState::Disconnected);
    CHECK(session.GetToken().empty());
}
