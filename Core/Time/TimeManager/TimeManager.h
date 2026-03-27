#pragma once
/**
 * @file TimeManager.h
 * @brief Game-time management: time scale, slow-motion, fixed-timestep, global timers.
 *
 * Features:
 *   - Separate game-time (affected by scale) and real-time clocks
 *   - SetTimeScale(): instant or interpolated (tween to slow-motion)
 *   - Per-actor time scale override (e.g. bullet-time affects only specific objects)
 *   - Fixed-timestep accumulator for physics (configurable step rate)
 *   - Named countdown / interval timers with callbacks
 *   - Frame counter, total game-time, total real-time
 *   - Pause / unpause (sets time scale to 0 and restores)
 *   - Time-scale zones (capsule/sphere volumes with local scale)
 *
 * Typical usage:
 * @code
 *   TimeManager tm;
 *   tm.Init(60.f);   // 60 Hz fixed step
 *   tm.SetTimeScale(0.3f, 0.2f);  // slow-mo over 0.2 s
 *   // main loop:
 *   tm.Advance(wallDt);
 *   float dt = tm.GetDeltaTime();
 *   while (tm.ConsumeFixedStep()) Physics::Step(tm.GetFixedStep());
 *   tm.CreateTimer("respawn", 5.f, []{ Respawn(); });
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Core {

struct TimerDesc {
    std::string              name;
    float                    duration{1.f};
    bool                     repeat{false};
    std::function<void()>    onFire;
};

struct TimerState {
    std::string  name;
    float        remaining{0.f};
    float        duration{0.f};
    bool         repeat{false};
    bool         paused{false};
    std::function<void()> onFire;
};

class TimeManager {
public:
    TimeManager();
    ~TimeManager();

    void Init(float fixedHz=60.f);
    void Shutdown();

    // Main update (call once per frame with wall-clock delta)
    void  Advance(float wallDt);

    // Time deltas
    float GetDeltaTime()    const;  ///< scaled game dt for this frame
    float GetRealDeltaTime()const;  ///< unscaled wall dt
    float GetFixedStep()    const;  ///< fixed physics step duration (e.g. 1/60)
    bool  ConsumeFixedStep();       ///< call in a while loop; returns true per step

    // Time scale
    void  SetTimeScale(float scale, float tweenDuration=0.f);
    float GetTimeScale() const;
    void  Pause();
    void  Unpause();
    bool  IsPaused() const;

    // Per-actor override
    void  SetActorTimeScale(uint32_t actorId, float scale);
    void  ClearActorTimeScale(uint32_t actorId);
    float GetActorTimeScale(uint32_t actorId) const;
    float GetActorDeltaTime(uint32_t actorId) const; ///< global * actor scale

    // Accumulators
    float    GetGameTime()  const;  ///< total scaled seconds since Init
    float    GetRealTime()  const;  ///< total wall seconds since Init
    uint64_t GetFrameCount()const;

    // Timers
    uint32_t CreateTimer(const TimerDesc& desc);
    uint32_t CreateTimer(const std::string& name, float duration,
                          std::function<void()> cb, bool repeat=false);
    void     PauseTimer(uint32_t id);
    void     ResumeTimer(uint32_t id);
    void     CancelTimer(uint32_t id);
    void     CancelTimer(const std::string& name);
    float    GetTimerRemaining(uint32_t id) const;
    bool     HasTimer(const std::string& name) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
