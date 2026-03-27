#pragma once
/**
 * @file SoundCue.h
 * @brief Multi-sample audio cue with random/sequential variation, pitch/vol range.
 *
 * Features:
 *   - CueDef: id, name, playMode (Random, Sequential, Shuffle), cooldown
 *   - RegisterCue(def) → bool
 *   - AddSample(cueId, clipName, weight, pitchMin, pitchMax, volMin, volMax)
 *   - RemoveSample(cueId, clipName)
 *   - Play(cueId, posX, posY, posZ) → instanceId: picks a sample per play-mode
 *   - Stop(instanceId)
 *   - IsPlaying(instanceId) → bool
 *   - Tick(dt): update cooldowns
 *   - SetCooldown(cueId, secs): minimum time between consecutive plays
 *   - GetLastPlayedClip(cueId) → string
 *   - GetSampleCount(cueId) → uint32_t
 *   - SetOnPlay(cueId, cb) / SetOnStop(cueId, cb)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class CuePlayMode : uint8_t { Random, Sequential, Shuffle };

struct CueDef {
    uint32_t    id;
    std::string name;
    CuePlayMode playMode{CuePlayMode::Random};
    float       cooldown{0.f};
};

struct CueSample {
    std::string clip;
    float       weight   {1.f};
    float       pitchMin {0.9f}, pitchMax{1.1f};
    float       volMin   {0.9f}, volMax  {1.0f};
};

class SoundCue {
public:
    SoundCue();
    ~SoundCue();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Registration
    bool RegisterCue(const CueDef& def);
    void AddSample  (uint32_t cueId, const CueSample& sample);
    void RemoveSample(uint32_t cueId, const std::string& clip);

    // Playback
    uint32_t Play(uint32_t cueId, float posX = 0, float posY = 0, float posZ = 0);
    void     Stop(uint32_t instanceId);
    bool     IsPlaying(uint32_t instanceId) const;

    // Per-frame
    void Tick(float dt);

    // Config
    void SetCooldown(uint32_t cueId, float secs);

    // Query
    std::string GetLastPlayedClip(uint32_t cueId) const;
    uint32_t    GetSampleCount   (uint32_t cueId) const;

    // Callbacks
    void SetOnPlay(uint32_t cueId, std::function<void(uint32_t instanceId)> cb);
    void SetOnStop(uint32_t cueId, std::function<void(uint32_t instanceId)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
