#pragma once
#include <string>
#include <cstdint>
#include <functional>
#include "Runtime/ECS/ECS.h"
#include "Engine/Net/NetContext.h"
#include "Engine/Sim/TickScheduler.h"

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
    EngineMode mode = EngineMode::Client;
    std::string assetRoot = "assets";
    uint32_t tickRate = 30;
    uint32_t maxTicks = 0; // 0 = unlimited (run forever), >0 = stop after N ticks
};

class Engine {
public:
    using FrameCallback = std::function<void(float dt)>;

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
    FrameCallback m_frameCallback;
};

} // namespace Engine::Core
