#pragma once
#include <atomic>
#include <chrono>

namespace NF {

/// @brief Central engine loop responsible for initialisation, fixed-timestep
///        updates, and graceful shutdown.
class EngineLoop {
public:
    /// @brief Initialise all engine subsystems. Must be called before Run().
    void Init();

    /// @brief Enter the blocking update loop. Returns when RequestExit() has
    ///        been called and the current frame finishes.
    void Run();

    /// @brief Tear down all engine subsystems. Must be called after Run()
    ///        returns.
    void Shutdown();

    /// @brief Signal the loop to stop after the current frame completes.
    void RequestExit() noexcept;

    /// @brief Fixed timestep in seconds used for physics / deterministic updates.
    static constexpr float FixedTimeStep = 1.0f / 60.0f;

private:
    /// @brief Execute one fixed-step logic tick.
    /// @param dt Fixed delta time in seconds.
    void FixedUpdate(float dt);

    /// @brief Execute one variable-rate render/visual tick.
    /// @param dt Elapsed seconds since the last frame.
    void Update(float dt);

    std::atomic<bool> m_Running{false};
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    TimePoint m_LastFrameTime{};
    float     m_Accumulator{0.0f};
};

} // namespace NF
