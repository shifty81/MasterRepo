#include "PCG/Weather/WeatherSystem/WeatherSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace PCG {

static WeatherParams DefaultParams(WeatherType t) {
    WeatherParams p;
    switch(t){
        case WeatherType::Clear:        p.sunIntensity=1.f; p.fogDensity=0.f; p.windSpeed=0.f; break;
        case WeatherType::PartlyCloudy: p.sunIntensity=0.8f; p.windSpeed=2.f; break;
        case WeatherType::Overcast:     p.sunIntensity=0.4f; p.fogDensity=0.02f; p.windSpeed=3.f; for(int i=0;i<3;++i) p.skyColour[i]=0.5f; break;
        case WeatherType::Fog:          p.sunIntensity=0.3f; p.fogDensity=0.15f; p.windSpeed=0.5f; for(int i=0;i<3;++i) p.skyColour[i]=0.8f; break;
        case WeatherType::Drizzle:      p.sunIntensity=0.5f; p.fogDensity=0.03f; p.precipitationDensity=0.2f; p.windSpeed=2.f; break;
        case WeatherType::Rain:         p.sunIntensity=0.3f; p.fogDensity=0.05f; p.precipitationDensity=0.7f; p.windSpeed=5.f; break;
        case WeatherType::HeavyRain:    p.sunIntensity=0.1f; p.fogDensity=0.1f; p.precipitationDensity=1.f; p.windSpeed=10.f; break;
        case WeatherType::Thunderstorm: p.sunIntensity=0.1f; p.fogDensity=0.12f; p.precipitationDensity=1.f; p.windSpeed=15.f; p.thunderInterval=8.f; break;
        case WeatherType::Snow:         p.sunIntensity=0.6f; p.fogDensity=0.04f; p.precipitationDensity=0.5f; p.windSpeed=3.f; for(int i=0;i<3;++i) p.skyColour[i]=0.85f; break;
        case WeatherType::Blizzard:     p.sunIntensity=0.1f; p.fogDensity=0.3f; p.precipitationDensity=1.f; p.windSpeed=25.f; break;
        case WeatherType::Sandstorm:    p.sunIntensity=0.2f; p.fogDensity=0.4f; p.windSpeed=30.f; p.skyColour[0]=0.7f; p.skyColour[1]=0.5f; p.skyColour[2]=0.2f; break;
        default: break;
    }
    return p;
}

static void LerpParams(WeatherParams& out, const WeatherParams& a, const WeatherParams& b, float t){
    float it=1-t;
    for(int i=0;i<3;++i){ out.skyColour[i]=a.skyColour[i]*it+b.skyColour[i]*t; out.fogColour[i]=a.fogColour[i]*it+b.fogColour[i]*t; }
    out.sunIntensity        =a.sunIntensity        *it+b.sunIntensity        *t;
    out.windSpeed           =a.windSpeed           *it+b.windSpeed           *t;
    out.fogDensity          =a.fogDensity          *it+b.fogDensity          *t;
    out.precipitationDensity=a.precipitationDensity*it+b.precipitationDensity*t;
    out.thunderInterval     =a.thunderInterval     *it+b.thunderInterval     *t;
    out.ambientOcclusion    =a.ambientOcclusion    *it+b.ambientOcclusion    *t;
}

struct WeatherSystem::Impl {
    WeatherState state;
    WeatherParams fromParams;
    WeatherParams targetParams;
    float transitionDuration{2.f};
    float transitionElapsed{0.f};

    std::unordered_map<uint8_t,WeatherParams> customParams;

    float timeOfDay{12.f};
    float timeAdvanceRate{0.f};
    float thunderTimer{0.f};

    std::function<void(WeatherType,WeatherType)> onChanged;
    std::function<void()>                        onThunder;

    WeatherParams ParamsFor(WeatherType t){
        auto it=customParams.find((uint8_t)t);
        return it!=customParams.end()?it->second:DefaultParams(t);
    }
};

WeatherSystem::WeatherSystem() : m_impl(new Impl()) {
    m_impl->fromParams  = DefaultParams(WeatherType::Clear);
    m_impl->targetParams= DefaultParams(WeatherType::Clear);
    m_impl->state.params= DefaultParams(WeatherType::Clear);
}
WeatherSystem::~WeatherSystem() { delete m_impl; }
void WeatherSystem::Init()     {}
void WeatherSystem::Shutdown() {}

void WeatherSystem::SetWeather(WeatherType type, float transSec){
    WeatherType from=m_impl->state.type;
    if(from==type) return;
    m_impl->fromParams         = m_impl->state.params;
    m_impl->targetParams       = m_impl->ParamsFor(type);
    m_impl->transitionDuration = std::max(0.01f, transSec);
    m_impl->transitionElapsed  = 0.f;
    m_impl->state.targetType   = type;
    m_impl->state.blendProgress= 0.f;
    if(m_impl->onChanged) m_impl->onChanged(from,type);
}

void WeatherSystem::SetTimeOfDay(float h){ m_impl->timeOfDay=std::fmod(h,24.f); }
void WeatherSystem::AdvanceTime(float rate){ m_impl->timeAdvanceRate=rate; }

WeatherState WeatherSystem::GetCurrentState() const { return m_impl->state; }
WeatherType  WeatherSystem::GetCurrentType()  const { return m_impl->state.type; }
const WeatherParams& WeatherSystem::GetParams() const { return m_impl->state.params; }

void WeatherSystem::Update(float dt){
    // Advance time
    if(m_impl->timeAdvanceRate>0.f){
        m_impl->timeOfDay=std::fmod(m_impl->timeOfDay+dt*m_impl->timeAdvanceRate,24.f);
        m_impl->state.timeOfDay=m_impl->timeOfDay;
    }
    // Blend weather
    if(m_impl->state.type!=m_impl->state.targetType){
        m_impl->transitionElapsed+=dt;
        float prog=std::min(1.f,m_impl->transitionElapsed/m_impl->transitionDuration);
        m_impl->state.blendProgress=prog;
        LerpParams(m_impl->state.params,m_impl->fromParams,m_impl->targetParams,prog);
        if(prog>=1.f){
            m_impl->state.type=m_impl->state.targetType;
            m_impl->state.params=m_impl->targetParams;
        }
    }
    // Thunder
    if(m_impl->state.params.thunderInterval>0.f){
        m_impl->thunderTimer+=dt;
        if(m_impl->thunderTimer>=m_impl->state.params.thunderInterval){
            m_impl->thunderTimer=0.f;
            if(m_impl->onThunder) m_impl->onThunder();
        }
    }
}

void WeatherSystem::SetWeatherParams(WeatherType type, const WeatherParams& params){
    m_impl->customParams[(uint8_t)type]=params;
}
void WeatherSystem::OnWeatherChanged(std::function<void(WeatherType,WeatherType)> cb){ m_impl->onChanged=std::move(cb); }
void WeatherSystem::OnThunder(std::function<void()> cb){ m_impl->onThunder=std::move(cb); }

} // namespace PCG
