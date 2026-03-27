#pragma once
/**
 * @file WeatherSystem.h
 * @brief Dynamic weather simulation: rain, snow, wind, fog, lightning, blending.
 *
 * Features:
 *   - SetWeatherState(type, intensity): apply a weather type immediately
 *   - BlendToWeather(type, intensity, duration): smooth transition over time
 *   - Tick(dt): advance transitions, update particle/fog parameters
 *   - GetCurrentType() → WeatherType
 *   - GetIntensity() → float [0,1]
 *   - GetWindVector() → Vec3: current world-space wind velocity
 *   - GetFogDensity() → float
 *   - GetPrecipitationRate() → float (particles/s per m²)
 *   - GetLightningActive() → bool / GetThunderDelay() → float
 *   - SetWindBase(dir, speed) / SetWindTurbulence(t)
 *   - SetOnWeatherChange(cb): callback(prev, next, intensity)
 *   - SetOnLightning(cb)
 *   - WeatherType: Clear, Cloudy, Rain, HeavyRain, Snow, Blizzard, Fog, Thunderstorm
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>

namespace Engine {

enum class WeatherType : uint8_t {
    Clear, Cloudy, Rain, HeavyRain, Snow, Blizzard, Fog, Thunderstorm
};

struct WeatherVec3 { float x, y, z; };

class WeatherSystem {
public:
    WeatherSystem();
    ~WeatherSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Control
    void SetWeatherState (WeatherType type, float intensity);
    void BlendToWeather  (WeatherType type, float intensity, float duration);

    // Per-frame
    void Tick(float dt);

    // Query
    WeatherType GetCurrentType       () const;
    float       GetIntensity         () const;
    WeatherVec3 GetWindVector        () const;
    float       GetFogDensity        () const;
    float       GetPrecipitationRate () const;
    bool        GetLightningActive   () const;
    float       GetThunderDelay      () const;

    // Wind config
    void SetWindBase       (WeatherVec3 dir, float speed);
    void SetWindTurbulence (float t);

    // Callbacks
    void SetOnWeatherChange(std::function<void(WeatherType prev,
                                               WeatherType next,
                                               float intensity)> cb);
    void SetOnLightning    (std::function<void()> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
