#include "Engine/World/Weather/WeatherSystem/WeatherSystem.h"
#include <algorithm>
#include <cmath>

namespace Engine {

struct WeatherSystem::Impl {
    WeatherType current{WeatherType::Clear};
    WeatherType target {WeatherType::Clear};
    float       intensity{0};
    float       targetIntensity{0};
    float       blendDur{0}, blendElapsed{0};
    bool        blending{false};

    WeatherVec3 windDir{1,0,0};
    float       windSpeed{0};
    float       windTurb{0.2f};
    float       windTime{0};

    float       lightningTimer{0};
    bool        lightningActive{false};
    float       thunderDelay{0};

    std::function<void(WeatherType,WeatherType,float)> onChange;
    std::function<void()>                              onLightning;

    float GetFogDensity() const {
        switch(current){
            case WeatherType::Fog:         return 0.05f+intensity*0.1f;
            case WeatherType::Thunderstorm:return 0.02f+intensity*0.05f;
            case WeatherType::Blizzard:    return 0.03f+intensity*0.06f;
            default: return 0;
        }
    }
    float GetPrecipitationRate() const {
        switch(current){
            case WeatherType::Rain:      return intensity*100;
            case WeatherType::HeavyRain: return intensity*300;
            case WeatherType::Snow:      return intensity*80;
            case WeatherType::Blizzard:  return intensity*250;
            case WeatherType::Thunderstorm:return intensity*200;
            default: return 0;
        }
    }
};

WeatherSystem::WeatherSystem(): m_impl(new Impl){}
WeatherSystem::~WeatherSystem(){ Shutdown(); delete m_impl; }
void WeatherSystem::Init(){}
void WeatherSystem::Shutdown(){}
void WeatherSystem::Reset(){ *m_impl=Impl{}; }

void WeatherSystem::SetWeatherState(WeatherType type, float intensity){
    WeatherType prev=m_impl->current;
    m_impl->current=type; m_impl->intensity=intensity;
    m_impl->target=type; m_impl->targetIntensity=intensity;
    m_impl->blending=false;
    if(m_impl->onChange&&prev!=type) m_impl->onChange(prev,type,intensity);
}
void WeatherSystem::BlendToWeather(WeatherType type, float intensity, float dur){
    m_impl->target=type; m_impl->targetIntensity=intensity;
    m_impl->blendDur=std::max(0.01f,dur); m_impl->blendElapsed=0;
    m_impl->blending=true;
}

void WeatherSystem::Tick(float dt){
    m_impl->windTime+=dt;
    if(m_impl->blending){
        m_impl->blendElapsed+=dt;
        float t=std::min(1.f,m_impl->blendElapsed/m_impl->blendDur);
        m_impl->intensity=m_impl->intensity*(1-t)+m_impl->targetIntensity*t;
        if(t>=1.f){
            WeatherType prev=m_impl->current;
            m_impl->current=m_impl->target;
            m_impl->intensity=m_impl->targetIntensity;
            m_impl->blending=false;
            if(m_impl->onChange&&prev!=m_impl->current)
                m_impl->onChange(prev,m_impl->current,m_impl->intensity);
        }
    }
    // Lightning
    if(m_impl->current==WeatherType::Thunderstorm){
        m_impl->lightningTimer-=dt;
        if(m_impl->lightningTimer<=0){
            m_impl->lightningActive=true;
            m_impl->thunderDelay=1.5f+2.f*std::sin(m_impl->windTime);
            m_impl->lightningTimer=3.f+4.f*std::abs(std::sin(m_impl->windTime*0.3f));
            if(m_impl->onLightning) m_impl->onLightning();
        } else { m_impl->lightningActive=false; }
        m_impl->thunderDelay=std::max(0.f,m_impl->thunderDelay-dt);
    } else { m_impl->lightningActive=false; }
}

WeatherType WeatherSystem::GetCurrentType() const { return m_impl->current; }
float WeatherSystem::GetIntensity() const { return m_impl->intensity; }
WeatherVec3 WeatherSystem::GetWindVector() const {
    float turb=m_impl->windTurb*std::sin(m_impl->windTime*2.3f);
    float s=m_impl->windSpeed*(1+turb);
    return {m_impl->windDir.x*s, 0, m_impl->windDir.z*s};
}
float WeatherSystem::GetFogDensity          () const { return m_impl->GetFogDensity(); }
float WeatherSystem::GetPrecipitationRate   () const { return m_impl->GetPrecipitationRate(); }
bool  WeatherSystem::GetLightningActive     () const { return m_impl->lightningActive; }
float WeatherSystem::GetThunderDelay        () const { return m_impl->thunderDelay; }

void WeatherSystem::SetWindBase(WeatherVec3 dir, float speed){
    m_impl->windDir=dir; m_impl->windSpeed=speed;
}
void WeatherSystem::SetWindTurbulence(float t){ m_impl->windTurb=t; }

void WeatherSystem::SetOnWeatherChange(
    std::function<void(WeatherType,WeatherType,float)> cb){ m_impl->onChange=cb; }
void WeatherSystem::SetOnLightning(std::function<void()> cb){ m_impl->onLightning=cb; }

} // namespace Engine
