// Entry point for the NovaForge Game runtime
#include "GameClientApp.h"
#include "Core/Logging/Log.h"

int main(int /*argc*/, char* /*argv*/[])
{
    NF::Logger::Log(NF::LogLevel::Info, "Game", "Starting NovaForge Game");

    NF::Game::GameClientApp app;
    if (!app.Init())
    {
        NF::Logger::Log(NF::LogLevel::Fatal, "Game", "GameClientApp init failed");
        return 1;
    }

    app.Run();
    app.Shutdown();

    NF::Logger::Log(NF::LogLevel::Info, "Game", "NovaForge Game exited cleanly");
    return 0;
}
