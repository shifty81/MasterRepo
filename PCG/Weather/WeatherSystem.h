#pragma once
/**
 * @file WeatherSystem.h
 * @brief Dynamic weather simulation for open-world environments.
 *
 * Manages:
 *   - WeatherState: all atmospheric parameters (precipitation, fog, wind,
 *     cloud cover, temperature, visibility)
 *   - WeatherPreset: named snapshots that can be blended
 *   - WeatherSystem: time-driven interpolation between presets; random
 *     natural transitions; callbacks for audio/VFX reaction; time-of-day
 *     integration (dawn/dusk modifiers)
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace PCG {

// ── Weather state (all values normalised 0..1 unless noted) ──────────────
struct WeatherState {
    float cloudCover{0.1f};       ///< 0 = clear  1 = overcast
    float rainIntensity{0.0f};    ///< 0 = dry     1 = heavy rain
    float snowIntensity{0.0f};    ///< 0 = none    1 = blizzard
    float fogDensity{0.0f};       ///< 0 = clear   1 = zero visibility
    float windSpeed{2.0f};        ///< m/s
    float windDirDeg{270.0f};     ///< degrees (270 = westerly)
    float temperature{15.0f};     ///< Celsius
    float humidity{0.5f};
    float lightningProbability{0.0f}; ///< probability per second
    float visibilityMetres{10000.0f};
    float fogColorR{0.8f}, fogColorG{0.85f}, fogColorB{0.9f};

    /// Linearly interpolate towards @p target by fraction @p t.
    WeatherState Lerp(const WeatherState& target, float t) const;
};

// ── Preset ────────────────────────────────────────────────────────────────
struct WeatherPreset {
    std::string  id;
    std::string  displayName;
    WeatherState state;
    float        minDurationSec{60.0f};  ///< Minimum hold before transition
    float        maxDurationSec{300.0f};
    float        weight{1.0f};           ///< Selection weight in random cycle
};

// ── Event ─────────────────────────────────────────────────────────────────
struct WeatherEvent {
    std::string fromPreset;
    std::string toPreset;
    float       blendDurationSec{10.0f};
};

using WeatherChangeCb    = std::function<void(const WeatherEvent&)>;
using WeatherTickCb      = std::function<void(const WeatherState&)>; ///< Called each Update

// ── System ───────────────────────────────────────────────────────────────
class WeatherSystem {
public:
    WeatherSystem();
    ~WeatherSystem();

    // ── preset management ─────────────────────────────────────
    void RegisterPreset(WeatherPreset preset);
    bool HasPreset(const std::string& id) const;
    size_t PresetCount() const;
    const WeatherPreset* GetPreset(const std::string& id) const;
    std::vector<std::string> PresetIds() const;

    // ── control ───────────────────────────────────────────────
    /// Start the system with a given initial preset.
    void Start(const std::string& presetId = "");
    void Stop();
    bool IsRunning() const;

    /// Manually transition to a new preset over @p blendDurationSec.
    void TransitionTo(const std::string& presetId, float blendDurationSec = 10.0f);

    /// Set fully custom weather state (bypasses preset system).
    void SetCustomState(const WeatherState& state);

    // ── update ────────────────────────────────────────────────
    /// Advance simulation by @p dt seconds; handles blending + auto-transition.
    void Update(float dt);

    // ── query ─────────────────────────────────────────────────
    const WeatherState& CurrentState() const;
    std::string CurrentPresetId() const;
    std::string TargetPresetId() const;
    float BlendFraction() const;  ///< 0 = at source, 1 = at target

    // ── time-of-day ───────────────────────────────────────────
    /// Set hour-of-day [0,24); adjusts temperature + visibility automatically.
    void SetTimeOfDay(float hour);
    float TimeOfDay() const;

    // ── random seed ───────────────────────────────────────────
    void SetSeed(uint64_t seed);

    // ── callbacks ─────────────────────────────────────────────
    void OnWeatherChange(WeatherChangeCb cb);
    void OnTick(WeatherTickCb cb);

    // ── built-in presets ──────────────────────────────────────
    static std::vector<WeatherPreset> BuiltinPresets();

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace PCG
