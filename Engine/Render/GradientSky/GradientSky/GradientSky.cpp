#include "Engine/Render/GradientSky/GradientSky/GradientSky.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

static float Lerp(float a, float b, float t) { return a+t*(b-a); }
static float Saturate(float v) { return std::max(0.f,std::min(1.f,v)); }

struct GradientSky::Impl {
    SkyColourSet colours;
    SkyBodyDesc  sun;
    SkyBodyDesc  moon;
    AtmosphereDesc atmosphere;
    float fogColour[3]{0.7f,0.7f,0.8f};
    float fogDensity{0.f};
    float timeOfDay{12.f};
    float dayDuration{1200.f};  ///< 20 min real-time = 24h
    bool  cycling{false};
    std::vector<SkyTimeKeyframe> keyframes;
    std::function<void(float)> onTimeChanged;

    void SampleKeyframes(float hour) {
        if (keyframes.size() < 2) return;
        // Find bracket
        uint32_t a=0, b=1;
        for (uint32_t i=0;i+1<keyframes.size();i++) {
            if (hour >= keyframes[i].hour && hour < keyframes[i+1].hour) {
                a=i; b=i+1; break;
            }
        }
        float t = (hour - keyframes[a].hour) /
                  (keyframes[b].hour - keyframes[a].hour + 1e-9f);
        t = Saturate(t);
        for(int i=0;i<3;i++) {
            colours.zenith [i]=Lerp(keyframes[a].colours.zenith [i],keyframes[b].colours.zenith [i],t);
            colours.horizon[i]=Lerp(keyframes[a].colours.horizon[i],keyframes[b].colours.horizon[i],t);
            colours.ground [i]=Lerp(keyframes[a].colours.ground [i],keyframes[b].colours.ground [i],t);
        }
    }
};

GradientSky::GradientSky()  : m_impl(new Impl) {}
GradientSky::~GradientSky() { Shutdown(); delete m_impl; }

void GradientSky::Init()     {}
void GradientSky::Shutdown() {}

void GradientSky::SetZenithColour (const float rgb[3]) { for(int i=0;i<3;i++) m_impl->colours.zenith [i]=rgb[i]; }
void GradientSky::SetHorizonColour(const float rgb[3]) { for(int i=0;i<3;i++) m_impl->colours.horizon[i]=rgb[i]; }
void GradientSky::SetGroundColour (const float rgb[3]) { for(int i=0;i<3;i++) m_impl->colours.ground [i]=rgb[i]; }
void GradientSky::SetSun          (const SkyBodyDesc& d){ m_impl->sun=d; }
void GradientSky::SetMoon         (const SkyBodyDesc& d){ m_impl->moon=d; }
void GradientSky::SetAtmosphere   (const AtmosphereDesc& d){ m_impl->atmosphere=d; }
void GradientSky::SetFogColour    (const float rgb[3]) { for(int i=0;i<3;i++) m_impl->fogColour[i]=rgb[i]; }
void GradientSky::SetFogDensity   (float d)            { m_impl->fogDensity=d; }

void GradientSky::Evaluate(const float worldDir[3], float outRGB[3]) const
{
    // y component determines up/down blend
    float up = worldDir[1]; // assumes Y-up
    up = Saturate(up * 0.5f + 0.5f); // remap -1..1 → 0..1

    // Blend from ground → horizon → zenith
    float* zen  = const_cast<float*>(m_impl->colours.zenith);
    float* hor  = const_cast<float*>(m_impl->colours.horizon);
    float* gnd  = const_cast<float*>(m_impl->colours.ground);

    for(int i=0;i<3;i++) {
        if (up > 0.5f) {
            float t = (up-0.5f)*2.f;
            outRGB[i] = Lerp(hor[i], zen[i], t);
        } else {
            float t = up*2.f;
            outRGB[i] = Lerp(gnd[i], hor[i], t);
        }
    }

    // Sun disc contribution
    if (m_impl->sun.enabled && m_impl->sun.intensity>0.f) {
        float cosAngle = 0.f;
        for(int i=0;i<3;i++) cosAngle += worldDir[i]*m_impl->sun.direction[i];
        float halfAngle = m_impl->sun.angularRadius * 3.14159f/180.f;
        float cosSun = std::cos(halfAngle);
        if (cosAngle > cosSun) {
            float blend = Saturate((cosAngle-cosSun)/(1.f-cosSun+1e-9f));
            for(int i=0;i<3;i++)
                outRGB[i] = Lerp(outRGB[i], m_impl->sun.colour[i]*m_impl->sun.intensity, blend);
        }
    }
}

void GradientSky::SetTimeOfDay  (float h) { m_impl->timeOfDay=h; m_impl->SampleKeyframes(h); }
float GradientSky::GetTimeOfDay() const   { return m_impl->timeOfDay; }
void GradientSky::SetDayDuration(float s) { m_impl->dayDuration=s; }
void GradientSky::SetCycling    (bool e)  { m_impl->cycling=e; }

void GradientSky::AddKeyframe   (const SkyTimeKeyframe& kf) { m_impl->keyframes.push_back(kf); }
void GradientSky::ClearKeyframes()                          { m_impl->keyframes.clear(); }
void GradientSky::SetOnTimeChanged(std::function<void(float)> cb) { m_impl->onTimeChanged=cb; }

void GradientSky::Tick(float dt)
{
    if (!m_impl->cycling) return;
    float hourPerSec = 24.f / (m_impl->dayDuration+1e-9f);
    m_impl->timeOfDay = std::fmod(m_impl->timeOfDay + hourPerSec*dt, 24.f);
    m_impl->SampleKeyframes(m_impl->timeOfDay);
    if (m_impl->onTimeChanged) m_impl->onTimeChanged(m_impl->timeOfDay);
}

} // namespace Engine
