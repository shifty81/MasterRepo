// Entry point for the NovaForge Editor
#include "Editor/Application/EditorApp.h"
#include "Core/Logging/Log.h"

int main(int argc, char* argv[])
{
    NF::Logger::Log(NF::LogLevel::Info, "Editor", "Starting NovaForge Editor");
    NF::Editor::EditorApp app;
    if (!app.Init())
    {
        NF::Logger::Log(NF::LogLevel::Fatal, "Editor", "Failed to initialise editor");
        return 1;
    }
    app.Run();
    app.Shutdown();
    return 0;
}
