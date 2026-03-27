#pragma once
/**
 * @file TimelineSystem.h
 * @brief Sequencer-style timeline: tracks, keyframes, playback, events, scrubbing.
 *
 * Features:
 *   - CreateTimeline(id) / DestroyTimeline(id)
 *   - AddTrack(timelineId, trackId, name): add a named track to a timeline
 *   - AddKeyframe(timelineId, trackId, time, value): insert a float keyframe
 *   - RemoveKeyframe(timelineId, trackId, time)
 *   - Play(timelineId) / Pause(timelineId) / Stop(timelineId)
 *   - Seek(timelineId, time): jump to a specific time
 *   - Tick(dt): advance all playing timelines
 *   - GetTime(timelineId) → float: current playback head
 *   - GetDuration(timelineId) → float: max keyframe time across all tracks
 *   - Sample(timelineId, trackId, time) → float: interpolated value
 *   - SetLooping(timelineId, on)
 *   - SetOnKeyframe(timelineId, trackId, time, cb): fire at exact time
 *   - SetOnEnd(timelineId, cb)
 *   - AddEvent(timelineId, time, tag): string event marker
 *   - SetOnEvent(timelineId, cb): callback(time, tag)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

class TimelineSystem {
public:
    TimelineSystem();
    ~TimelineSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Timeline management
    void CreateTimeline (uint32_t id);
    void DestroyTimeline(uint32_t id);

    // Track / keyframe management
    void AddTrack    (uint32_t timelineId, uint32_t trackId, const std::string& name);
    void AddKeyframe (uint32_t timelineId, uint32_t trackId, float time, float value);
    void RemoveKeyframe(uint32_t timelineId, uint32_t trackId, float time);

    // Playback control
    void Play (uint32_t timelineId);
    void Pause(uint32_t timelineId);
    void Stop (uint32_t timelineId);
    void Seek (uint32_t timelineId, float time);
    void SetLooping(uint32_t timelineId, bool on);

    // Per-frame
    void Tick(float dt);

    // Query
    float GetTime    (uint32_t timelineId) const;
    float GetDuration(uint32_t timelineId) const;
    float Sample     (uint32_t timelineId, uint32_t trackId, float time) const;
    bool  IsPlaying  (uint32_t timelineId) const;

    // Events
    void AddEvent  (uint32_t timelineId, float time, const std::string& tag);
    void SetOnEvent(uint32_t timelineId,
                    std::function<void(float time, const std::string& tag)> cb);

    // Callbacks
    void SetOnKeyframe(uint32_t timelineId, uint32_t trackId, float time,
                       std::function<void()> cb);
    void SetOnEnd     (uint32_t timelineId, std::function<void()> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
