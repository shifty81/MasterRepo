// Entry point for the NovaForge Game runtime
#include "Game/App/Bootstrap.h"
#include "Game/App/Orchestrator.h"
#include "Core/Logging/Log.h"
#include "Renderer/RHI/RenderDevice.h"
#include <memory>

int main(int /*argc*/, char* /*argv*/[])
{
    NF::Logger::Log(NF::LogLevel::Info, "Game", "Starting NovaForge Game");

    // Bootstrap — resolve paths and establish session
    NF::Game::Bootstrap bootstrap;
    NF::Game::BootstrapConfig bsCfg;
    bsCfg.RepoRoot = ".";          // resolved relative to CWD at runtime

    auto result = bootstrap.Run(bsCfg);
    if (!result.Success)
    {
        NF::Logger::Log(NF::LogLevel::Fatal, "Game",
                        "Bootstrap failed: " + result.Message);
        return 1;
    }

    // Renderer
    auto renderDevice = std::make_unique<NF::RenderDevice>();
    if (!renderDevice->Init(NF::GraphicsAPI::Null))
    {
        NF::Logger::Log(NF::LogLevel::Fatal, "Game", "RenderDevice init failed");
        return 1;
    }

    // Orchestrator
    NF::Game::Orchestrator orchestrator;
    if (!orchestrator.Init(renderDevice.get()))
    {
        NF::Logger::Log(NF::LogLevel::Fatal, "Game", "Orchestrator init failed");
        return 1;
    }

    // Fixed-timestep game loop
    constexpr float kDt = 1.0f / 60.0f;
    constexpr int   kMaxFrames = 0; // 0 = run until external signal; non-zero for headless testing
    int frame = 0;
    while (kMaxFrames == 0 ? true : frame < kMaxFrames)
    {
        orchestrator.Tick(kDt);
        ++frame;
    }

    orchestrator.Shutdown();
    renderDevice->Shutdown();
    bootstrap.Shutdown();

    NF::Logger::Log(NF::LogLevel::Info, "Game", "NovaForge Game exited cleanly");
    return 0;
}

