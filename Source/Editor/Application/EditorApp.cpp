#include "Editor/Application/EditorApp.h"
#include "Core/Logging/Log.h"
#include <thread>

namespace NF::Editor {

bool EditorApp::Init() {
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Init");
    m_RenderDevice = std::make_unique<RenderDevice>();
    if (!m_RenderDevice->Init(GraphicsAPI::Null)) {
        Logger::Log(LogLevel::Error, "Editor", "RenderDevice init failed");
        return false;
    }
    m_Level.Load("untitled.level");
    m_Running = true;
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Init complete");
    return true;
}

void EditorApp::Run() {
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Run – entering editor loop");
    while (m_Running) {
        m_RenderDevice->BeginFrame();
        m_RenderDevice->Clear(0.18f, 0.18f, 0.18f, 1.f);
        m_Level.Update(0.016f);
        m_RenderDevice->EndFrame();
        // Single iteration stub; a real loop would poll events here.
        m_Running = false;
    }
}

void EditorApp::Shutdown() {
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Shutdown");
    m_Level.Unload();
    m_RenderDevice->Shutdown();
}

} // namespace NF::Editor
