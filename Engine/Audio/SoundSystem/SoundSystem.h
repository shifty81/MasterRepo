#pragma once
/**
 * @file SoundSystem.h
 * @brief 2D/3D sound playback with instance pooling, fading, and looping.
 *
 * Features:
 *   - Sound clip registry: load/cache by name from file path
 *   - 2D (non-spatialized) and 3D (spatialized by listener distance) playback
 *   - Per-instance: volume, pitch, loop, fade-in / fade-out
 *   - Sound groups (buses): Master, Music, SFX, Voice, Ambient with volume scalars
 *   - Instance pooling: fixed capacity reuse of completed instances
 *   - Priority-based eviction when pool is full
 *   - Listener position / orientation for 3D panning/attenuation
 *   - Pause / resume all, or by group
 *   - Sound cues: ordered / random selection from a pool of clips
 *   - On-complete callback per instance
 *
 * Typical usage:
 * @code
 *   SoundSystem ss;
 *   ss.Init(32);   // 32-instance pool
 *   ss.RegisterClip("gunshot", "Audio/sfx/gunshot.wav");
 *   uint32_t id = ss.Play2D("gunshot", SoundGroup::SFX, 0.8f);
 *   ss.Play3D("footstep", position, SoundGroup::SFX);
 *   ss.SetListenerTransform(camPos, camForward, camUp);
 *   ss.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class SoundGroup : uint8_t { Master=0, Music, SFX, Voice, Ambient, COUNT };

struct SoundClipDesc {
    std::string id;
    std::string filePath;
    float       defaultVolume{1.f};
    float       defaultPitch {1.f};
    bool        is3D{false};
    float       minDist{1.f};
    float       maxDist{50.f};
};

struct SoundCueDesc {
    std::string              cueId;
    std::vector<std::string> clipIds;
    bool                     randomOrder{true};
};

struct SoundPlayParams {
    float       volume{1.f};
    float       pitch {1.f};
    bool        loop  {false};
    float       fadeIn{0.f};   ///< seconds
    SoundGroup  group {SoundGroup::SFX};
    int32_t     priority{0};   ///< higher = less likely to be evicted
};

enum class SoundState : uint8_t { Playing=0, Paused, Fading, Stopped };

struct SoundInstance {
    uint32_t    instanceId{0};
    std::string clipId;
    SoundState  state{SoundState::Stopped};
    float       volume{1.f};
    float       pitch {1.f};
    float       elapsed{0.f};
    float       duration{0.f};  ///< 0 = unknown / streaming
    bool        loop  {false};
    float       fadeTimer{0.f};
    float       fadeDuration{0.f};
    float       fadeTargetVol{0.f};
    SoundGroup  group{SoundGroup::SFX};
    float       position[3]{};
    bool        is3D{false};
};

class SoundSystem {
public:
    SoundSystem();
    ~SoundSystem();

    void Init(uint32_t maxInstances=32);
    void Shutdown();
    void Tick(float dt);

    // Clip / cue registry
    void RegisterClip(const SoundClipDesc& desc);
    void RegisterClip(const std::string& id, const std::string& path);
    void RegisterCue (const SoundCueDesc& desc);
    void UnregisterClip(const std::string& id);
    bool HasClip(const std::string& id) const;

    // Playback
    uint32_t Play2D(const std::string& clipId,
                    SoundGroup group=SoundGroup::SFX,
                    float volume=1.f, float pitch=1.f,
                    bool loop=false, float fadeIn=0.f);
    uint32_t Play3D(const std::string& clipId,
                    const float position[3],
                    SoundGroup group=SoundGroup::SFX,
                    float volume=1.f, float pitch=1.f,
                    bool loop=false, float fadeIn=0.f);
    uint32_t Play  (const std::string& clipId, const SoundPlayParams& params,
                    const float* pos3D=nullptr);
    uint32_t PlayCue(const std::string& cueId, SoundGroup group=SoundGroup::SFX);

    // Instance control
    void Stop     (uint32_t id, float fadeOut=0.f);
    void StopAll  (SoundGroup group, float fadeOut=0.f);
    void Pause    (uint32_t id);
    void Resume   (uint32_t id);
    void PauseGroup (SoundGroup group);
    void ResumeGroup(SoundGroup group);
    void SetVolume(uint32_t id, float volume);
    void SetPitch (uint32_t id, float pitch);
    void SetPosition(uint32_t id, const float pos[3]);
    bool IsPlaying(uint32_t id) const;

    // Group volume
    void  SetGroupVolume(SoundGroup group, float volume);
    float GetGroupVolume(SoundGroup group) const;

    // Listener
    void SetListenerTransform(const float pos[3],
                               const float forward[3],
                               const float up[3]);

    // Callback
    void SetOnComplete(uint32_t id, std::function<void()> cb);

    // Query
    const SoundInstance* GetInstance(uint32_t id) const;
    uint32_t ActiveCount() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
