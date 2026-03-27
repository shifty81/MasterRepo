#pragma once
/**
 * @file CameraShakeSystem.h
 * @brief Procedural camera shake using a trauma/decay model.
 *
 * Features:
 *   - Trauma model: add trauma [0,1]; shake = trauma²; decays each frame
 *   - Perlin-noise positional and rotational shake offsets
 *   - Simultaneous named shake presets (explosion, gunfire, footstep, earthquake)
 *   - Per-shake frequency, amplitude, and decay rate
 *   - Directional shake bias (e.g. push back on recoil)
 *   - Additive multi-source: stack trauma from multiple sources
 *   - Output: position delta [3] + rotation delta (Euler radians [3])
 *   - Fade-in support for sustained effects (e.g. standing next to engine)
 *   - Spring-return dampening for physically-plausible settle
 *
 * Typical usage:
 * @code
 *   CameraShakeSystem css;
 *   css.Init();
 *   css.RegisterPreset({"explosion", 0.8f, 0.6f, 25.f, 1.8f});
 *   css.RegisterPreset({"gunfire",   0.3f, 0.2f, 40.f, 5.0f});
 *   // on event:
 *   css.AddTrauma(0.7f, "explosion");
 *   // per-frame:
 *   css.Tick(dt);
 *   ShakeOffset off = css.GetOffset();
 *   camera.position += off.position;
 *   camera.rotation += off.rotation;
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct ShakePreset {
    std::string  name;
    float        maxPositionAmplitude{0.3f}; ///< metres
    float        maxRotationAmplitude{0.1f}; ///< radians
    float        frequency{20.f};            ///< noise frequency (Hz)
    float        decayRate{3.f};             ///< trauma decay per second
    float        fadeInTime{0.f};            ///< 0 = instant
    float        directionBias[3]{};         ///< normalised directional push
    bool         affectsPosition{true};
    bool         affectsRotation{true};
};

struct ShakeOffset {
    float position[3]{};   ///< world-space delta to apply to camera pos
    float rotation[3]{};   ///< Euler angle delta (pitch, yaw, roll)
    float traumaLevel{0.f};
};

struct ActiveShake {
    uint32_t    id{0};
    std::string presetName;
    float       traumaContrib{0.f};
    float       elapsed{0.f};
    float       fadeInTime{0.f};
};

class CameraShakeSystem {
public:
    CameraShakeSystem();
    ~CameraShakeSystem();

    void Init();
    void Shutdown();
    void Tick(float dt);

    // Preset management
    void RegisterPreset(const ShakePreset& preset);
    void UnregisterPreset(const std::string& name);

    // Trigger shake
    uint32_t AddTrauma(float amount, const std::string& presetName="default");
    uint32_t AddTraumaAtPosition(float amount, const std::string& presetName,
                                  const float worldPos[3],
                                  const float cameraPos[3],
                                  float maxRadius=20.f);
    void     StopShake(uint32_t shakeId);
    void     StopAll();

    // Global scale (for accessibility / settings)
    void     SetGlobalScale(float scale);
    float    GetGlobalScale() const;

    // Output
    ShakeOffset GetOffset() const;
    float       GetCurrentTrauma() const;

    // Callback (optional)
    using ShakeEndCb = std::function<void(uint32_t shakeId)>;
    void SetOnShakeEnd(ShakeEndCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
