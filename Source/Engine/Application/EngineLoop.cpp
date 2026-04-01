#include "Engine/Application/EngineLoop.h"
#include "Core/Logging/Log.h"
#include <thread>

namespace NF {

void EngineLoop::Init() {
    Logger::Log(LogLevel::Info, "Engine", "EngineLoop::Init");
    m_Running       = false;
    m_Accumulator   = 0.0f;
    m_LastFrameTime = Clock::now();
}

void EngineLoop::Run() {
    Logger::Log(LogLevel::Info, "Engine", "EngineLoop::Run – entering main loop");
    m_Running       = true;
    m_LastFrameTime = Clock::now();

    while (m_Running.load(std::memory_order_relaxed)) {
        const TimePoint now = Clock::now();
        const float dt = std::chrono::duration<float>(now - m_LastFrameTime).count();
        m_LastFrameTime = now;

        // Clamp to prevent spiral-of-death on very slow frames.
        const float clampedDt = (dt > 0.25f) ? 0.25f : dt;
        m_Accumulator += clampedDt;

        while (m_Accumulator >= FixedTimeStep) {
            FixedUpdate(FixedTimeStep);
            m_Accumulator -= FixedTimeStep;
        }

        Update(clampedDt);

        // Yield to avoid burning a core when there is nothing to do.
        std::this_thread::yield();
    }
}

void EngineLoop::Shutdown() {
    Logger::Log(LogLevel::Info, "Engine", "EngineLoop::Shutdown");
    m_Running = false;
}

void EngineLoop::RequestExit() noexcept {
    m_Running.store(false, std::memory_order_relaxed);
}

void EngineLoop::FixedUpdate([[maybe_unused]] float dt) {
    // Subsystem fixed-rate updates (physics, networking, …) go here.
}

void EngineLoop::Update([[maybe_unused]] float dt) {
    // Per-frame updates (input, rendering, …) go here.
}

} // namespace NF
