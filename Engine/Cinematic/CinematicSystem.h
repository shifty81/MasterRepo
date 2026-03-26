#pragma once
/**
 * @file CinematicSystem.h
 * @brief Spline-based camera path system for cutscenes, trailers, and
 *        in-engine cinematics.
 *
 * Supports:
 *   - Catmull-Rom spline paths with arbitrary keyframes (position + rotation)
 *   - Per-keyframe field-of-view and exposure overrides
 *   - Playback control: play, pause, seek, loop
 *   - Sequence manager: chain multiple shots with optional black-bar letterbox
 *
 * Typical usage:
 * @code
 *   CinematicSystem cinema;
 *   cinema.Init();
 *   uint32_t seqId = cinema.CreateSequence("intro");
 *   cinema.AddKeyframe(seqId, {0.f, {0,50,0}, {0,0,0,1}, 60.f});
 *   cinema.AddKeyframe(seqId, {5.f, {100,20,0}, {0,0.7f,0,0.7f}, 45.f});
 *   cinema.Play(seqId);
 *   cinema.Tick(deltaSeconds);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

// ── A single camera keyframe ──────────────────────────────────────────────────

struct CinemaKeyframe {
    float    time{0.f};          ///< time in seconds from sequence start
    float    position[3]{};      ///< world position xyz
    float    rotation[4]{0,0,0,1}; ///< quaternion xyzw
    float    fov{60.f};          ///< vertical field of view degrees
    float    exposure{1.f};      ///< exposure multiplier
};

// ── Playback state ────────────────────────────────────────────────────────────

enum class CinemaState : uint8_t { Idle, Playing, Paused, Finished };

// ── A cinematic sequence (a series of keyframes) ──────────────────────────────

struct CinemaSequence {
    uint32_t                  id{0};
    std::string               name;
    std::vector<CinemaKeyframe> keyframes;
    float                     duration{0.f};   ///< auto-computed from last keyframe
    bool                      loop{false};
    bool                      letterbox{true}; ///< 2.35:1 black bars
    CinemaState               state{CinemaState::Idle};
    float                     playhead{0.f};   ///< current playback time (seconds)
};

// ── Interpolated camera pose (output each tick) ───────────────────────────────

struct CameraPose {
    float position[3]{};
    float rotation[4]{0,0,0,1};
    float fov{60.f};
    float exposure{1.f};
};

// ── CinematicSystem ───────────────────────────────────────────────────────────

class CinematicSystem {
public:
    CinematicSystem();
    ~CinematicSystem();

    void Init();
    void Shutdown();

    // ── Sequence management ───────────────────────────────────────────────────

    uint32_t    CreateSequence(const std::string& name);
    void        DestroySequence(uint32_t seqId);
    void        AddKeyframe(uint32_t seqId, const CinemaKeyframe& kf);
    void        ClearKeyframes(uint32_t seqId);
    CinemaSequence GetSequence(uint32_t seqId) const;
    std::vector<CinemaSequence> AllSequences() const;

    // ── Playback ──────────────────────────────────────────────────────────────

    void        Play(uint32_t seqId);
    void        Pause(uint32_t seqId);
    void        Stop(uint32_t seqId);
    void        Seek(uint32_t seqId, float timeSeconds);
    void        SetLoop(uint32_t seqId, bool loop);

    /// Advance all active sequences.  Returns interpolated pose of the
    /// currently playing sequence (or last one if multiple).
    CameraPose  Tick(float deltaSeconds);

    bool        IsPlaying(uint32_t seqId) const;
    uint32_t    ActiveSequenceId()        const; ///< 0 = none

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnSequenceStart(std::function<void(uint32_t seqId)> cb);
    void OnSequenceEnd(std::function<void(uint32_t seqId)> cb);
    void OnKeyframeReached(std::function<void(uint32_t seqId, uint32_t kfIdx)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
