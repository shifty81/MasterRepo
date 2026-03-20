#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>

namespace UI {

// ──────────────────────────────────────────────────────────────
// Easing functions
// ──────────────────────────────────────────────────────────────

enum class Easing {
    Linear,
    EaseIn,    // quadratic ease-in
    EaseOut,   // quadratic ease-out
    EaseInOut, // smooth step
    Elastic,
    Bounce
};

float ApplyEasing(float t, Easing easing);

// ──────────────────────────────────────────────────────────────
// Keyframe — one target value at a given normalised time [0,1]
// ──────────────────────────────────────────────────────────────

struct Keyframe {
    float  time   = 0.0f;  // [0, 1]
    float  value  = 0.0f;
    Easing easing = Easing::Linear;
};

// ──────────────────────────────────────────────────────────────
// AnimationTrack — animates a single named float property
// ──────────────────────────────────────────────────────────────

struct AnimationTrack {
    std::string          property; // e.g. "opacity", "x", "y", "scale"
    std::vector<Keyframe> keyframes;

    // Evaluate the interpolated value at normalised time t in [0,1]
    float Evaluate(float t) const;
};

// ──────────────────────────────────────────────────────────────
// UIAnimation — a named group of tracks
// ──────────────────────────────────────────────────────────────

class UIAnimation {
public:
    UIAnimation() = default;
    explicit UIAnimation(const std::string& name, float durationSec = 0.3f);

    // Configuration
    void SetName(const std::string& name)   { m_name = name; }
    void SetDuration(float sec)             { m_duration = sec; }
    void SetLooping(bool loop)              { m_loop = loop; }
    const std::string& Name() const         { return m_name; }
    float Duration() const                  { return m_duration; }

    // Track management
    void AddTrack(const AnimationTrack& track);
    void ClearTracks();
    const std::vector<AnimationTrack>& Tracks() const { return m_tracks; }

    // Playback
    void Play();
    void Pause();
    void Stop();
    void Rewind();
    bool IsPlaying()  const { return m_playing; }
    bool IsFinished() const { return m_finished; }

    // Advance time by dt seconds; fires callbacks as appropriate
    void Update(float dt);

    // Evaluated values at current time (property → value)
    float GetProperty(const std::string& prop) const;
    const std::unordered_map<std::string, float>& Properties() const { return m_current; }

    // Callbacks
    using VoidCallback = std::function<void()>;
    void OnFinish(VoidCallback cb)   { m_onFinish = std::move(cb); }
    void OnLoop(VoidCallback cb)     { m_onLoop   = std::move(cb); }

private:
    std::string                            m_name;
    float                                  m_duration  = 0.3f;
    bool                                   m_loop      = false;
    bool                                   m_playing   = false;
    bool                                   m_finished  = false;
    float                                  m_time      = 0.0f; // current elapsed
    std::vector<AnimationTrack>            m_tracks;
    std::unordered_map<std::string, float> m_current;
    VoidCallback                           m_onFinish;
    VoidCallback                           m_onLoop;

    void EvaluateAll();
};

// ──────────────────────────────────────────────────────────────
// UIAnimator — manages a collection of named animations
// ──────────────────────────────────────────────────────────────

class UIAnimator {
public:
    void AddAnimation(const UIAnimation& anim);
    UIAnimation* Get(const std::string& name);
    void Play(const std::string& name);
    void Stop(const std::string& name);
    void Update(float dt);
    void Clear();

    // Convenience: fade in / fade out helpers
    static UIAnimation MakeFadeIn (float durationSec = 0.25f);
    static UIAnimation MakeFadeOut(float durationSec = 0.25f);
    static UIAnimation MakeSlideIn(float fromX, float toX, float durationSec = 0.3f);

private:
    std::unordered_map<std::string, UIAnimation> m_anims;
};

} // namespace UI
