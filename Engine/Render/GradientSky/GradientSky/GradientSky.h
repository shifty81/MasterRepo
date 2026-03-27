#pragma once
/**
 * @file GradientSky.h
 * @brief Sky gradient: zenith/horizon/ground colours, sun disc, atmospheric scattering params.
 *
 * Features:
 *   - Three-colour sky gradient: zenith, horizon, ground (below horizon)
 *   - Gradient evaluation at any world direction
 *   - Sun disc: direction, angular radius, colour, intensity
 *   - Moon disc: direction, angular radius, colour, intensity
 *   - Simple Rayleigh scattering: beta coefficient, scattering direction
 *   - Mie scattering: haze factor, g-coefficient
 *   - Time-of-day: map 0-24h to zenith/horizon/sun colour sets
 *   - Day/night cycle: delta-time advance
 *   - Fog integration: horizon fog colour + density
 *   - Evaluate(worldDir) → linear RGB colour
 *   - OnTimeChanged callback
 *
 * Typical usage:
 * @code
 *   GradientSky sky;
 *   sky.Init();
 *   sky.SetZenithColour({0.1f,0.2f,0.8f});
 *   sky.SetHorizonColour({0.7f,0.8f,0.9f});
 *   sky.SetSunDirection({0.3f,-0.7f,0.6f});
 *   float col[3]; sky.Evaluate({0,1,0}, col);
 *   sky.SetTimeOfDay(14.5f);
 *   sky.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct SkyColourSet {
    float zenith [3]{0.1f,0.2f,0.8f};
    float horizon[3]{0.7f,0.8f,0.9f};
    float ground [3]{0.3f,0.25f,0.2f};
};

struct SkyBodyDesc {
    float direction[3]{0,-1,0};
    float angularRadius{0.5f};    ///< degrees
    float colour[3]{1,1,1};
    float intensity{1.f};
    bool  enabled{true};
};

struct AtmosphereDesc {
    float rayleighBeta[3]{5.5e-3f, 13.e-3f, 22.4e-3f};
    float mieHaze     {0.1f};
    float mieG        {0.8f};
    bool  enabled     {true};
};

struct SkyTimeKeyframe {
    float          hour{12.f};   ///< 0-24
    SkyColourSet   colours;
    SkyBodyDesc    sun;
};

class GradientSky {
public:
    GradientSky();
    ~GradientSky();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);                    ///< advances time-of-day if cycling enabled

    // Static colours
    void SetZenithColour (const float rgb[3]);
    void SetHorizonColour(const float rgb[3]);
    void SetGroundColour (const float rgb[3]);

    // Sun / Moon
    void SetSun (const SkyBodyDesc& d);
    void SetMoon(const SkyBodyDesc& d);

    // Atmosphere
    void SetAtmosphere(const AtmosphereDesc& d);

    // Fog
    void  SetFogColour (const float rgb[3]);
    void  SetFogDensity(float density);

    // Evaluation
    void Evaluate(const float worldDir[3], float outRGB[3]) const;

    // Time of day
    void  SetTimeOfDay(float hour);           ///< 0-24
    float GetTimeOfDay()            const;
    void  SetDayDuration(float seconds);      ///< real-time seconds for 24h cycle
    void  SetCycling(bool enabled);

    // Keyframes
    void AddKeyframe    (const SkyTimeKeyframe& kf);
    void ClearKeyframes ();

    void SetOnTimeChanged(std::function<void(float hour)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
