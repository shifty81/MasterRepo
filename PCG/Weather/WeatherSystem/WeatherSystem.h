#pragma once
/**
 * @file WeatherSystem.h
 * @brief Procedural weather system with blending, transitions, and effects.
 *
 * Weather states (Clear, Cloudy, Rain, Thunderstorm, Fog, Snow, Blizzard)
 * blend smoothly with cross-fade.  Weather drives:
 *   - Sky colour & sun intensity
 *   - Wind speed/direction
 *   - Precipitation type, density, sound
 *   - Visibility distance (fog density)
 *   - GPU particle emitter enablement
 *
 * Typical usage:
 * @code
 *   WeatherSystem ws;
 *   ws.Init();
 *   ws.SetWeather(WeatherType::Rain, 3.f);  // 3s blend
 *   // Each frame:
 *   ws.Update(dt);
 *   WeatherState s = ws.GetCurrentState();
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace PCG {

enum class WeatherType : uint8_t {
    Clear=0, PartlyCloudy, Overcast, Fog,
    Drizzle, Rain, HeavyRain, Thunderstorm,
    Snow, Blizzard, Sandstorm,
    COUNT
};

struct WeatherParams {
    float skyColour[3]{0.53f,0.81f,0.92f};
    float sunIntensity{1.f};
    float windSpeed{0.f};
    float windDir[2]{1.f,0.f};      ///< (x,z) unit vector
    float fogDensity{0.f};
    float fogColour[3]{0.9f,0.9f,0.9f};
    float precipitationDensity{0.f};
    float precipitationSize{0.01f};
    float thunderInterval{0.f};     ///< 0 = no thunder
    float ambientOcclusion{0.3f};
};

struct WeatherState {
    WeatherType  type{WeatherType::Clear};
    WeatherType  targetType{WeatherType::Clear};
    WeatherParams params;
    float         blendProgress{1.f};  ///< 1 = fully transitioned
    float         timeOfDay{12.f};     ///< 0-24 hours
};

class WeatherSystem {
public:
    WeatherSystem();
    ~WeatherSystem();

    void Init();
    void Shutdown();

    void SetWeather(WeatherType type, float transitionSeconds = 2.f);
    void SetTimeOfDay(float hour);           ///< 0-24
    void AdvanceTime(float gameHoursPerSecond);

    WeatherState      GetCurrentState() const;
    WeatherType       GetCurrentType()  const;
    const WeatherParams& GetParams()    const;

    void Update(float dt);

    /// Register custom param overrides for a weather type
    void SetWeatherParams(WeatherType type, const WeatherParams& params);

    void OnWeatherChanged(std::function<void(WeatherType from, WeatherType to)> cb);
    void OnThunder(std::function<void()> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace PCG
