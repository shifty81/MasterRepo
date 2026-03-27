#include "Engine/Animation/Layer/AnimationLayer/AnimationLayer.h"
#include <algorithm>
#include <unordered_map>

namespace Engine {

struct LayerState {
    LayerMode   mode{LayerMode::Override};
    float       weight{1.f};
    float       targetWeight{1.f};
    float       blendTimer{0.f};
    float       blendDuration{0.f};
    bool        blendingOut{false};
    std::vector<uint32_t> boneMask;
    std::string currentClip;
    float       clipTime{0.f};
    bool        playing{false};
    bool        loop{false};
    float       clipLength{1.f}; // stub: 1 second
    std::function<void()> onComplete;
};

struct AnimationLayer::Impl {
    std::unordered_map<uint32_t, LayerState> layers;
};

AnimationLayer::AnimationLayer()  { m_impl = new Impl; }
AnimationLayer::~AnimationLayer() { delete m_impl; }
void AnimationLayer::Init    () {}
void AnimationLayer::Shutdown() { Reset(); }
void AnimationLayer::Reset   () { m_impl->layers.clear(); }

bool AnimationLayer::AddLayer(uint32_t id, LayerMode mode, float weight,
                               const std::vector<uint32_t>& boneMask) {
    if (m_impl->layers.count(id)) return false;
    LayerState s; s.mode = mode; s.weight = weight; s.targetWeight = weight; s.boneMask = boneMask;
    m_impl->layers[id] = std::move(s);
    return true;
}
void AnimationLayer::RemoveLayer(uint32_t id) { m_impl->layers.erase(id); }

void  AnimationLayer::SetLayerWeight(uint32_t id, float w) {
    auto it = m_impl->layers.find(id); if (it != m_impl->layers.end()) { it->second.weight = w; it->second.targetWeight = w; }
}
float AnimationLayer::GetLayerWeight(uint32_t id) const {
    auto it = m_impl->layers.find(id); return it != m_impl->layers.end() ? it->second.weight : 0;
}
void AnimationLayer::SetLayerMode(uint32_t id, LayerMode m) {
    auto it = m_impl->layers.find(id); if (it != m_impl->layers.end()) it->second.mode = m;
}
void AnimationLayer::SetBoneMask(uint32_t id, const std::vector<uint32_t>& bones) {
    auto it = m_impl->layers.find(id); if (it != m_impl->layers.end()) it->second.boneMask = bones;
}

bool AnimationLayer::PlayClip(uint32_t id, const std::string& clip, bool loop) {
    auto it = m_impl->layers.find(id); if (it == m_impl->layers.end()) return false;
    auto& s = it->second; s.currentClip = clip; s.clipTime = 0; s.playing = true; s.loop = loop;
    return true;
}
void AnimationLayer::StopClip(uint32_t id) {
    auto it = m_impl->layers.find(id); if (it != m_impl->layers.end()) it->second.playing = false;
}

void AnimationLayer::BlendIn(uint32_t id, float duration) {
    auto it = m_impl->layers.find(id); if (it == m_impl->layers.end()) return;
    auto& s = it->second; s.weight = 0; s.targetWeight = 1; s.blendDuration = duration > 0 ? duration : 1e-4f;
    s.blendTimer = 0; s.blendingOut = false;
}
void AnimationLayer::BlendOut(uint32_t id, float duration) {
    auto it = m_impl->layers.find(id); if (it == m_impl->layers.end()) return;
    auto& s = it->second; s.targetWeight = 0; s.blendDuration = duration > 0 ? duration : 1e-4f;
    s.blendTimer = 0; s.blendingOut = true;
}

void AnimationLayer::Tick(float dt) {
    for (auto& [id, s] : m_impl->layers) {
        // blend
        if (s.blendDuration > 0) {
            s.blendTimer += dt;
            float t = s.blendTimer / s.blendDuration;
            if (t >= 1.f) { t = 1.f; s.blendDuration = 0; }
            s.weight = s.blendingOut ? (1.f - t) : t * s.targetWeight;
            if (s.blendingOut && t >= 1.f) { s.playing = false; s.blendingOut = false; }
        }
        // advance clip
        if (!s.playing) continue;
        s.clipTime += dt;
        if (s.clipTime >= s.clipLength) {
            if (s.loop) { s.clipTime = std::fmod(s.clipTime, s.clipLength); }
            else { s.clipTime = s.clipLength; s.playing = false; if (s.onComplete) s.onComplete(); }
        }
    }
}

std::string AnimationLayer::GetCurrentClip(uint32_t id) const {
    auto it = m_impl->layers.find(id); return it != m_impl->layers.end() ? it->second.currentClip : "";
}
float AnimationLayer::GetClipTime(uint32_t id) const {
    auto it = m_impl->layers.find(id); return it != m_impl->layers.end() ? it->second.clipTime : 0;
}
bool AnimationLayer::IsPlaying(uint32_t id) const {
    auto it = m_impl->layers.find(id); return it != m_impl->layers.end() && it->second.playing;
}
uint32_t AnimationLayer::GetLayerCount() const { return (uint32_t)m_impl->layers.size(); }
void AnimationLayer::SetOnComplete(uint32_t id, std::function<void()> cb) {
    auto it = m_impl->layers.find(id); if (it != m_impl->layers.end()) it->second.onComplete = std::move(cb);
}

} // namespace Engine
