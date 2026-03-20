#include "UI/Animation/UIAnimation.h"
#include <cmath>
#include <algorithm>

namespace UI {

// ── easing ────────────────────────────────────────────────────

float ApplyEasing(float t, Easing easing) {
    t = std::clamp(t, 0.0f, 1.0f);
    switch (easing) {
        case Easing::Linear:    return t;
        case Easing::EaseIn:    return t * t;
        case Easing::EaseOut:   return t * (2.0f - t);
        case Easing::EaseInOut: return t < 0.5f ? 2.0f*t*t : -1.0f+(4.0f-2.0f*t)*t;
        case Easing::Elastic: {
            if (t == 0.0f || t == 1.0f) return t;
            float p = 0.3f;
            return std::pow(2.0f, -10.0f*t) *
                   std::sin((t - p/4.0f) * (2.0f*3.14159265f) / p) + 1.0f;
        }
        case Easing::Bounce: {
            if (t < 1.0f/2.75f)      return 7.5625f*t*t;
            else if (t < 2.0f/2.75f) { t -= 1.5f/2.75f;   return 7.5625f*t*t + 0.75f; }
            else if (t < 2.5f/2.75f) { t -= 2.25f/2.75f;  return 7.5625f*t*t + 0.9375f; }
            else                     { t -= 2.625f/2.75f;  return 7.5625f*t*t + 0.984375f; }
        }
    }
    return t;
}

// ── AnimationTrack ─────────────────────────────────────────────

float AnimationTrack::Evaluate(float t) const {
    if (keyframes.empty()) return 0.0f;
    if (t <= keyframes.front().time) return keyframes.front().value;
    if (t >= keyframes.back().time)  return keyframes.back().value;

    for (size_t i = 1; i < keyframes.size(); ++i) {
        const auto& a = keyframes[i-1];
        const auto& b = keyframes[i];
        if (t >= a.time && t <= b.time) {
            float span = b.time - a.time;
            float local = (span > 0.0f) ? (t - a.time) / span : 0.0f;
            float eased = ApplyEasing(local, b.easing);
            return a.value + eased * (b.value - a.value);
        }
    }
    return keyframes.back().value;
}

// ── UIAnimation ────────────────────────────────────────────────

UIAnimation::UIAnimation(const std::string& name, float durationSec)
    : m_name(name), m_duration(durationSec) {}

void UIAnimation::AddTrack(const AnimationTrack& track) {
    m_tracks.push_back(track);
    m_current[track.property] = 0.0f;
}
void UIAnimation::ClearTracks() { m_tracks.clear(); m_current.clear(); }

void UIAnimation::Play()   { m_playing = true; m_finished = false; }
void UIAnimation::Pause()  { m_playing = false; }
void UIAnimation::Stop()   { m_playing = false; m_finished = true; m_time = 0.0f; EvaluateAll(); }
void UIAnimation::Rewind() { m_time = 0.0f; m_finished = false; EvaluateAll(); }

void UIAnimation::Update(float dt) {
    if (!m_playing || m_finished) return;
    m_time += dt;
    if (m_time >= m_duration) {
        if (m_loop) {
            m_time = std::fmod(m_time, m_duration);
            EvaluateAll();
            if (m_onLoop) m_onLoop();
        } else {
            m_time = m_duration;
            EvaluateAll();
            m_playing  = false;
            m_finished = true;
            if (m_onFinish) m_onFinish();
        }
    } else {
        EvaluateAll();
    }
}

float UIAnimation::GetProperty(const std::string& prop) const {
    auto it = m_current.find(prop);
    return (it != m_current.end()) ? it->second : 0.0f;
}

void UIAnimation::EvaluateAll() {
    float t = (m_duration > 0.0f) ? std::clamp(m_time / m_duration, 0.0f, 1.0f) : 1.0f;
    for (const auto& track : m_tracks)
        m_current[track.property] = track.Evaluate(t);
}

// ── UIAnimator ────────────────────────────────────────────────

void UIAnimator::AddAnimation(const UIAnimation& anim) {
    m_anims[anim.Name()] = anim;
}

UIAnimation* UIAnimator::Get(const std::string& name) {
    auto it = m_anims.find(name);
    return (it != m_anims.end()) ? &it->second : nullptr;
}

void UIAnimator::Play(const std::string& name) {
    if (auto* a = Get(name)) a->Play();
}

void UIAnimator::Stop(const std::string& name) {
    if (auto* a = Get(name)) a->Stop();
}

void UIAnimator::Update(float dt) {
    for (auto& [_, anim] : m_anims) anim.Update(dt);
}

void UIAnimator::Clear() { m_anims.clear(); }

// ── factory helpers ────────────────────────────────────────────

UIAnimation UIAnimator::MakeFadeIn(float durationSec) {
    UIAnimation anim("fade_in", durationSec);
    AnimationTrack track;
    track.property = "opacity";
    track.keyframes = {{0.0f, 0.0f, Easing::EaseOut},
                       {1.0f, 1.0f, Easing::Linear}};
    anim.AddTrack(track);
    return anim;
}

UIAnimation UIAnimator::MakeFadeOut(float durationSec) {
    UIAnimation anim("fade_out", durationSec);
    AnimationTrack track;
    track.property = "opacity";
    track.keyframes = {{0.0f, 1.0f, Easing::EaseIn},
                       {1.0f, 0.0f, Easing::Linear}};
    anim.AddTrack(track);
    return anim;
}

UIAnimation UIAnimator::MakeSlideIn(float fromX, float toX, float durationSec) {
    UIAnimation anim("slide_in", durationSec);
    AnimationTrack track;
    track.property = "x";
    track.keyframes = {{0.0f, fromX, Easing::EaseOut},
                       {1.0f, toX,   Easing::Linear}};
    anim.AddTrack(track);
    return anim;
}

} // namespace UI
