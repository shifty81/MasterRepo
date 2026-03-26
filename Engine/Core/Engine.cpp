#include "Engine/Core/Engine.h"
#include "Engine/Core/Logger.h"
#include "Engine/Window/Window.h"
#include "Engine/Render/Renderer.h"
#include <chrono>
#include <thread>

namespace Engine::Core {

Engine::Engine(const EngineConfig& cfg) : m_config(cfg) {
}

Engine::~Engine() {
    Shutdown();
}

void Engine::InitCore() {
    Logger::Init();
    Logger::Info("Engine core initialized");
    m_running = true;
}

void Engine::InitRender() {
    if (m_config.mode == EngineMode::Server) {
        Logger::Info("Server mode: rendering disabled");
        return;
    }

    // Create GLFW window
    ::Engine::Window::WindowConfig winCfg;
    winCfg.title  = m_config.windowTitle;
    winCfg.width  = 1280;
    winCfg.height = 720;
    winCfg.vsync  = true;

    m_window = std::make_unique<::Engine::Window::Window>(winCfg);
    if (!m_window->Init()) {
        Logger::Error("InitRender — window creation failed; running headless");
        m_window.reset();
    }

    // Only create the renderer when we have a valid window context
    if (m_window) {
        ::Engine::Render::RenderConfig renderCfg;
        m_renderer = std::make_unique<::Engine::Render::Renderer>(renderCfg);
        m_renderer->Init(m_window->GetWidth(), m_window->GetHeight());
    }

    Logger::Info("Render system initialized");
}

void Engine::InitUI() {
    if (m_config.mode == EngineMode::Server) {
        Logger::Info("Server mode: UI disabled");
        return;
    }
    Logger::Info("UI system initialized");
}

void Engine::InitECS() {
    Logger::Info("ECS initialized (empty world)");
}

void Engine::InitNetworking() {
    net::NetRole role = net::NetRole::None;
    if (m_config.mode == EngineMode::Server) {
        role = net::NetRole::DedicatedServer;
    } else if (m_config.mode == EngineMode::Client) {
        role = net::NetRole::Client;
    }
    m_net.SetRole(role);
    if (role != net::NetRole::None) m_net.Start();
    Logger::Info("Networking initialized");
}

void Engine::InitEditor() {
    if (m_config.mode != EngineMode::Editor) {
        return;
    }
    Logger::Info("Editor tools initialized");
}

void Engine::Run() {
    m_scheduler.SetTickRate(m_config.tickRate);

    switch (m_config.mode) {
        case EngineMode::Editor: Logger::Info("Running Editor");  break;
        case EngineMode::Client: Logger::Info("Running Client");  break;
        case EngineMode::Server: Logger::Info("Running Server");  break;
    }

    // ESC or OS window-close button stops the loop
    if (m_window) {
        m_window->onKey = [this](int key, bool pressed) {
            // GLFW_KEY_ESCAPE is 256 in all GLFW 3.x versions
            constexpr int kKeyEscape = 256;
            if (key == kKeyEscape && pressed) m_running = false;
        };
    }

    const auto kFrameDuration =
        std::chrono::duration<double>(1.0 / static_cast<double>(m_config.tickRate));

    while (m_running) {
        auto frameStart = std::chrono::steady_clock::now();

        // Poll OS events and check for window-close request
        if (m_window) {
            m_window->PollEvents();
            if (m_window->ShouldClose())
                m_running = false;
        }
        if (!m_running) break;

        m_net.Poll();
        float dt = static_cast<float>(kFrameDuration.count());
        m_scheduler.Tick([this](float d) {
            m_world.Update(d);
        });
        if (m_frameCallback) m_frameCallback(dt);

        // Render: clear → game draws → present
        if (m_renderer && m_window) {
            m_renderer->BeginFrame();
            m_renderer->Clear();
            m_renderer->EndFrame();
            m_window->SwapBuffers();
        }

        m_tickCount++;
        if (m_config.maxTicks > 0 && m_tickCount >= m_config.maxTicks) {
            m_running = false;
        }

        // Frame-rate cap: sleep for the remainder of this frame's budget
        auto elapsed = std::chrono::steady_clock::now() - frameStart;
        if (elapsed < kFrameDuration)
            std::this_thread::sleep_for(kFrameDuration - elapsed);
    }
}

void Engine::RunEditor() {
    Run();
}

void Engine::RunClient() {
    Run();
}

void Engine::RunServer() {
    Run();
}

bool Engine::Running() const {
    return m_running;
}

void Engine::Shutdown() {
    if (m_running) {
        Logger::Info("Engine shutting down");
        m_net.Stop();
        m_running = false;
    }
    // Tear down render resources before destroying the window context
    if (m_renderer) {
        m_renderer->Shutdown();
        m_renderer.reset();
    }
    if (m_window) {
        m_window->Shutdown();
        m_window.reset();
    }
}

bool Engine::Can(Capability cap) const {
    switch (cap) {
        case Capability::AssetWrite:
            return m_config.mode == EngineMode::Editor;
        case Capability::Rendering:
            return m_config.mode != EngineMode::Server;
        case Capability::Physics:
            return true;
        case Capability::GraphEdit:
            return m_config.mode == EngineMode::Editor;
        case Capability::GraphExecute:
            return true;
        case Capability::NetAuthority:
            return m_config.mode == EngineMode::Server;
        case Capability::HotReload:
            return m_config.mode == EngineMode::Editor;
    }
    return false;
}

} // namespace Engine::Core
