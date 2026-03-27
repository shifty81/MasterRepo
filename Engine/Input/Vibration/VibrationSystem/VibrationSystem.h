#pragma once
/**
 * @file VibrationSystem.h
 * @brief Gamepad haptic feedback: waveform patterns, per-motor control, event queue, decay.
 *
 * Features:
 *   - RegisterController(id): register a gamepad slot
 *   - PlayPattern(controllerId, pattern): play a named or custom vibration pattern
 *   - PlayImpulse(controllerId, leftStrength, rightStrength, durationSec): one-shot rumble
 *   - PlayWave(controllerId, motorMask, samples[], count, sampleRate): raw waveform
 *   - StopAll(controllerId): immediately silence motors
 *   - SetBaseIntensity(controllerId, intensity): scale all output [0,1]
 *   - Tick(dt): advance patterns and apply decay, call SetMotors callback
 *   - SetOnMotors(cb): callback(controllerId, leftStrength, rightStrength) to feed platform
 *   - RegisterPattern(name, pattern): store a reusable VibrationPattern
 *   - GetCurrentStrength(controllerId) → (left, right)
 *   - IsPlaying(controllerId) → bool
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum VibrationMotor : uint8_t { MotorLeft=1, MotorRight=2, MotorBoth=3 };

struct VibrationKeyframe {
    float timeSec;
    float left{0};
    float right{0};
};

struct VibrationPattern {
    std::string                  name;
    std::vector<VibrationKeyframe> keyframes;
    bool                         loop{false};
};

class VibrationSystem {
public:
    VibrationSystem();
    ~VibrationSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();
    void Tick    (float dt);

    // Controller management
    void RegisterController  (uint32_t id);
    void UnregisterController(uint32_t id);

    // Playback
    void PlayPattern(uint32_t controllerId, const std::string& patternName);
    void PlayImpulse(uint32_t controllerId, float left, float right, float durationSec);
    void PlayWave   (uint32_t controllerId, VibrationMotor motor,
                     const float* samples, uint32_t count, uint32_t sampleRate=60);
    void StopAll    (uint32_t controllerId);

    // Pattern library
    void RegisterPattern(const VibrationPattern& pattern);

    // Config
    void SetBaseIntensity(uint32_t controllerId, float intensity);

    // Query
    void GetCurrentStrength(uint32_t controllerId, float& outLeft, float& outRight) const;
    bool IsPlaying(uint32_t controllerId) const;

    // Platform callback
    void SetOnMotors(std::function<void(uint32_t id, float left, float right)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
