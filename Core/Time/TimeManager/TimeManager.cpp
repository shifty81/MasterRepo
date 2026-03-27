#include "Core/Time/TimeManager/TimeManager.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Core {

struct TimeManager::Impl {
    float  fixedStep{1.f/60.f};
    float  timeScale{1.f};
    float  prevTimeScale{1.f};  // for unpause
    float  accumulator{0.f};
    float  gameTime{0.f};
    float  realTime{0.f};
    float  gameDt{0.f};
    float  realDt{0.f};
    uint64_t frame{0};
    bool   paused{false};

    // Tween for time scale
    float  scaleTarget{1.f};
    float  scaleTweenDur{0.f};
    float  scaleTweenElapsed{0.f};
    float  scaleFrom{1.f};
    bool   scaleTweening{false};

    // Per-actor
    std::unordered_map<uint32_t,float> actorScales;

    // Timers
    struct TimerEntry {
        uint32_t id{0};
        TimerState state;
    };
    std::vector<TimerEntry> timers;
    uint32_t nextTimerId{1};

    void UpdateTimers(float dt) {
        std::vector<uint32_t> fired;
        for (auto& te : timers) {
            auto& ts = te.state;
            if (ts.paused) continue;
            ts.remaining -= dt;
            if (ts.remaining <= 0.f) {
                fired.push_back(te.id);
                if (ts.onFire) ts.onFire();
                if (ts.repeat) ts.remaining += ts.duration;
            }
        }
        timers.erase(std::remove_if(timers.begin(),timers.end(),[&](auto& te){
            return !te.state.repeat && te.state.remaining <= 0.f;
        }), timers.end());
    }
};

TimeManager::TimeManager()  : m_impl(new Impl) {}
TimeManager::~TimeManager() { Shutdown(); delete m_impl; }

void TimeManager::Init(float fixedHz)
{
    m_impl->fixedStep = 1.f / fixedHz;
    m_impl->accumulator = 0.f;
    m_impl->gameTime = 0.f;
    m_impl->realTime = 0.f;
    m_impl->frame    = 0;
    m_impl->timeScale = 1.f;
    m_impl->paused   = false;
}

void TimeManager::Shutdown() { m_impl->timers.clear(); }

void TimeManager::Advance(float wallDt)
{
    m_impl->realDt   = wallDt;
    m_impl->realTime += wallDt;
    m_impl->frame++;

    // Scale tween
    if (m_impl->scaleTweening) {
        m_impl->scaleTweenElapsed += wallDt;
        float t = m_impl->scaleTweenDur > 0.f
            ? std::min(1.f, m_impl->scaleTweenElapsed / m_impl->scaleTweenDur)
            : 1.f;
        m_impl->timeScale = m_impl->scaleFrom + (m_impl->scaleTarget - m_impl->scaleFrom)*t;
        if (t >= 1.f) { m_impl->scaleTweening=false; m_impl->timeScale=m_impl->scaleTarget; }
    }

    float scale = m_impl->paused ? 0.f : m_impl->timeScale;
    m_impl->gameDt   = wallDt * scale;
    m_impl->gameTime += m_impl->gameDt;
    m_impl->accumulator += m_impl->gameDt;

    m_impl->UpdateTimers(m_impl->gameDt);
}

float TimeManager::GetDeltaTime()    const { return m_impl->gameDt; }
float TimeManager::GetRealDeltaTime()const { return m_impl->realDt; }
float TimeManager::GetFixedStep()    const { return m_impl->fixedStep; }

bool TimeManager::ConsumeFixedStep()
{
    if (m_impl->accumulator >= m_impl->fixedStep) {
        m_impl->accumulator -= m_impl->fixedStep;
        return true;
    }
    return false;
}

void TimeManager::SetTimeScale(float scale, float tweenDur)
{
    if (tweenDur > 0.f) {
        m_impl->scaleFrom         = m_impl->timeScale;
        m_impl->scaleTarget       = scale;
        m_impl->scaleTweenDur     = tweenDur;
        m_impl->scaleTweenElapsed = 0.f;
        m_impl->scaleTweening     = true;
    } else {
        m_impl->timeScale    = scale;
        m_impl->scaleTweening= false;
    }
}

float TimeManager::GetTimeScale() const { return m_impl->timeScale; }

void TimeManager::Pause()
{
    m_impl->prevTimeScale = m_impl->timeScale;
    m_impl->paused = true;
}

void TimeManager::Unpause()
{
    m_impl->paused    = false;
    m_impl->timeScale = m_impl->prevTimeScale;
}

bool TimeManager::IsPaused() const { return m_impl->paused; }

void TimeManager::SetActorTimeScale(uint32_t actorId, float scale)
{ m_impl->actorScales[actorId] = scale; }

void TimeManager::ClearActorTimeScale(uint32_t actorId)
{ m_impl->actorScales.erase(actorId); }

float TimeManager::GetActorTimeScale(uint32_t actorId) const
{
    auto it = m_impl->actorScales.find(actorId);
    return it != m_impl->actorScales.end() ? it->second : 1.f;
}

float TimeManager::GetActorDeltaTime(uint32_t actorId) const
{
    return m_impl->gameDt * GetActorTimeScale(actorId);
}

float    TimeManager::GetGameTime()   const { return m_impl->gameTime; }
float    TimeManager::GetRealTime()   const { return m_impl->realTime; }
uint64_t TimeManager::GetFrameCount() const { return m_impl->frame; }

uint32_t TimeManager::CreateTimer(const TimerDesc& desc)
{
    uint32_t id = m_impl->nextTimerId++;
    Impl::TimerEntry te;
    te.id = id;
    te.state.name      = desc.name;
    te.state.duration  = desc.duration;
    te.state.remaining = desc.duration;
    te.state.repeat    = desc.repeat;
    te.state.onFire    = desc.onFire;
    m_impl->timers.push_back(te);
    return id;
}

uint32_t TimeManager::CreateTimer(const std::string& name, float duration,
                                    std::function<void()> cb, bool repeat)
{
    TimerDesc d; d.name=name; d.duration=duration; d.repeat=repeat; d.onFire=cb;
    return CreateTimer(d);
}

void TimeManager::PauseTimer(uint32_t id)
{
    for (auto& te : m_impl->timers) if (te.id==id) { te.state.paused=true; break; }
}

void TimeManager::ResumeTimer(uint32_t id)
{
    for (auto& te : m_impl->timers) if (te.id==id) { te.state.paused=false; break; }
}

void TimeManager::CancelTimer(uint32_t id)
{
    auto& v = m_impl->timers;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& te){ return te.id==id; }), v.end());
}

void TimeManager::CancelTimer(const std::string& name)
{
    auto& v = m_impl->timers;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& te){ return te.state.name==name; }), v.end());
}

float TimeManager::GetTimerRemaining(uint32_t id) const
{
    for (auto& te : m_impl->timers) if (te.id==id) return te.state.remaining;
    return 0.f;
}

bool TimeManager::HasTimer(const std::string& name) const
{
    for (auto& te : m_impl->timers) if (te.state.name==name) return true;
    return false;
}

} // namespace Core
