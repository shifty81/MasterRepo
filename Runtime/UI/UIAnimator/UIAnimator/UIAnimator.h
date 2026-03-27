#pragma once
/**
 * @file UIAnimator.h
 * @brief Keyframe-driven UI property animation: position, scale, alpha, color, sequence.
 *
 * Features:
 *   - PropertyType: X, Y, ScaleX, ScaleY, Alpha, R, G, B
 *   - CreateAnimation(animId) / DestroyAnimation(animId)
 *   - AddKeyframe(animId, property, time, value, easeIn, easeOut) → bool
 *   - SetLoop(animId, on) / SetPingPong(animId, on)
 *   - SetDuration(animId) → derived from keyframes, or explicit
 *   - Play(animId, targetId) / Stop(animId, targetId) / Pause(animId, targetId)
 *   - IsPlaying(animId, targetId) → bool
 *   - GetValue(animId, targetId, property) → float: current evaluated value
 *   - Tick(dt): advance all playing animations
 *   - SetOnComplete(animId, cb): callback(targetId) when animation finishes
 *   - SetOnLoop(animId, cb): callback(targetId) on each loop
 *   - SequencePlay(animIds[], targetId): play animations back-to-back
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Runtime {

enum class UIAnimProperty : uint8_t { X, Y, ScaleX, ScaleY, Alpha, R, G, B };

class UIAnimator {
public:
    UIAnimator();
    ~UIAnimator();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Animation management
    void CreateAnimation (uint32_t animId);
    void DestroyAnimation(uint32_t animId);

    // Keyframes
    bool AddKeyframe(uint32_t animId, UIAnimProperty prop,
                     float time, float value,
                     float easeIn = 0.f, float easeOut = 0.f);

    // Config
    void SetLoop    (uint32_t animId, bool on);
    void SetPingPong(uint32_t animId, bool on);

    // Playback
    void Play (uint32_t animId, uint32_t targetId);
    void Stop (uint32_t animId, uint32_t targetId);
    void Pause(uint32_t animId, uint32_t targetId);
    bool IsPlaying(uint32_t animId, uint32_t targetId) const;

    // Sequence
    void SequencePlay(const std::vector<uint32_t>& animIds, uint32_t targetId);

    // Per-frame
    void Tick(float dt);

    // Query
    float GetValue(uint32_t animId, uint32_t targetId,
                   UIAnimProperty prop) const;

    // Callbacks
    void SetOnComplete(uint32_t animId,
                       std::function<void(uint32_t targetId)> cb);
    void SetOnLoop    (uint32_t animId,
                       std::function<void(uint32_t targetId)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
