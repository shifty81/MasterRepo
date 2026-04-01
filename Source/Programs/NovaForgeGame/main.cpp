// Entry point for the NovaForge Game runtime
#include "Engine/Application/EngineLoop.h"
#include "Core/Logging/Log.h"

int main(int argc, char* argv[])
{
    NF::Logger::Log(NF::LogLevel::Info, "Game", "Starting NovaForge Game");
    NF::EngineLoop loop;
    loop.Init();
    loop.Run();
    loop.Shutdown();
    return 0;
}
