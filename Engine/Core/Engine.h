#pragma once
#include <string>
#include <cstdint>
#include <functional>
#include <memory>
#include "Runtime/ECS/ECS.h"
#include "Engine/Net/NetContext.h"
#include "Engine/Sim/TickScheduler.h"

// Forward declarations to avoid pulling GLFW/render headers into every TU
namespace Engine::Window { class Window; }
namespace Engine::Render { class Renderer; }

namespace Engine::Core {

enum class RuntimeRole {
    Editor,
    Client,
    Server
};

enum class EngineMode {
    Editor,
    Client,
    Server
};

enum class Capability {
    AssetWrite,
    Rendering,
    Physics,
    GraphEdit,
    GraphExecute,
    NetAuthority,
    HotReload
};

struct EngineConfig {
    EngineMode  mode        = EngineMode::Client;
    std::string assetRoot   = "assets";
    std::string windowTitle = "NovaForge";
    uint32_t    tickRate    = 30;
    uint32_t    maxTicks    = 0; // 0 = unlimited (run forever), >0 = stop after N ticks
};

class Engine {
public:
    using FrameCallback  = std::function<void(float dt)>;
    using RenderCallback = std::function<void(int w, int h)>;

    explicit Engine(const EngineConfig& cfg);
    ~Engine();

    void InitCore();
    void InitRender();
    void InitUI();
    void InitECS();
    void InitNetworking();
    void InitEditor();

    void Run();
    void RunEditor();
    void RunClient();
    void RunServer();

    bool Running() const;
    void Shutdown();

    bool Can(Capability cap) const;

    /** Register a per-frame callback invoked once per tick.
     *  Useful for editor UI drawing, asset hot-reload polling, etc. */
    void SetFrameCallback(FrameCallback cb) { m_frameCallback = std::move(cb); }

    /** Register a render callback invoked each frame between Clear() and
     *  SwapBuffers(). Use this to draw custom OpenGL content into the window
     *  (e.g. a game HUD). The callback receives the current framebuffer
     *  width and height. */
    void SetRenderCallback(RenderCallback cb) { m_renderCallback = std::move(cb); }

    /** Number of ticks executed so far. */
    uint64_t TickCount() const { return m_tickCount; }

    const EngineConfig& Config() const { return m_config; }

    ::Runtime::ECS::World& GetWorld() { return m_world; }
    net::NetContext& GetNet() { return m_net; }
    Sim::TickScheduler& GetScheduler() { return m_scheduler; }

private:
    EngineConfig m_config;
    bool m_running = false;
    uint64_t m_tickCount = 0;
    ::Runtime::ECS::World m_world;
    net::NetContext m_net;
    Sim::TickScheduler m_scheduler;
    FrameCallback  m_frameCallback;
    RenderCallback m_renderCallback;
    std::unique_ptr<::Engine::Window::Window>   m_window;
    std::unique_ptr<::Engine::Render::Renderer> m_renderer;
};

} // namespace Engine::Core
