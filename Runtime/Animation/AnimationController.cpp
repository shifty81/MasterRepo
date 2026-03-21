#include "Runtime/Animation/AnimationController.h"
#include <unordered_map>
#include <algorithm>

namespace Runtime {

struct AnimLayerKey {
    EntityID entity;
    int      layer;
    bool operator==(const AnimLayerKey& o) const {
        return entity == o.entity && layer == o.layer;
    }
};

struct AnimLayerKeyHash {
    size_t operator()(const AnimLayerKey& k) const {
        return std::hash<uint64_t>{}(
            (static_cast<uint64_t>(k.entity) << 16) |
            static_cast<uint64_t>(static_cast<uint32_t>(k.layer)));
    }
};

struct AnimationController::Impl {
    std::unordered_map<std::string, float>   clipDurations;
    std::unordered_map<AnimLayerKey, AnimState, AnimLayerKeyHash> states;
    std::vector<AnimEventCallback>           callbacks;
};

AnimationController::AnimationController() : m_impl(new Impl()) {}
AnimationController::~AnimationController() { delete m_impl; }

void AnimationController::RegisterClip(const std::string& name, float duration) {
    m_impl->clipDurations[name] = duration;
}

bool AnimationController::HasClip(const std::string& name) const {
    return m_impl->clipDurations.count(name) > 0;
}

size_t AnimationController::ClipCount() const {
    return m_impl->clipDurations.size();
}

void AnimationController::Play(EntityID entity, const std::string& clip,
                                AnimPlayMode mode, float speed, int layer)
{
    auto it = m_impl->clipDurations.find(clip);
    float dur = it != m_impl->clipDurations.end() ? it->second : 1.0f;

    AnimState& s = m_impl->states[{entity, layer}];
    s.entityId  = entity;
    s.clipName  = clip;
    s.time      = 0.0f;
    s.duration  = dur;
    s.speed     = speed;
    s.weight    = 1.0f;
    s.mode      = mode;
    s.playing   = true;
    s.finished  = false;
}

void AnimationController::Stop(EntityID entity, int layer) {
    auto it = m_impl->states.find({entity, layer});
    if (it != m_impl->states.end()) it->second.playing = false;
}

void AnimationController::StopAll(EntityID entity) {
    for (auto& [key, state] : m_impl->states)
        if (key.entity == entity) state.playing = false;
}

void AnimationController::SetSpeed(EntityID entity, float speed, int layer) {
    auto it = m_impl->states.find({entity, layer});
    if (it != m_impl->states.end()) it->second.speed = speed;
}

void AnimationController::SetWeight(EntityID entity, float weight, int layer) {
    auto it = m_impl->states.find({entity, layer});
    if (it != m_impl->states.end())
        it->second.weight = std::max(0.0f, std::min(1.0f, weight));
}

void AnimationController::SeekTo(EntityID entity, float time, int layer) {
    auto it = m_impl->states.find({entity, layer});
    if (it != m_impl->states.end()) it->second.time = time;
}

bool AnimationController::IsPlaying(EntityID entity, int layer) const {
    auto it = m_impl->states.find({entity, layer});
    return it != m_impl->states.end() && it->second.playing;
}

void AnimationController::Update(float dt) {
    for (auto& [key, s] : m_impl->states) {
        if (!s.playing || s.duration <= 0.0f) continue;
        s.time += dt * s.speed;

        switch (s.mode) {
        case AnimPlayMode::Loop:
            while (s.time >= s.duration) {
                s.time -= s.duration;
                AnimEvent ev{s.entityId, s.clipName, true};
                for (auto& cb : m_impl->callbacks) cb(ev);
            }
            break;
        case AnimPlayMode::Once:
            if (s.time >= s.duration) {
                s.time    = s.duration;
                s.playing = false;
                s.finished = true;
                AnimEvent ev{s.entityId, s.clipName, false};
                for (auto& cb : m_impl->callbacks) cb(ev);
            }
            break;
        case AnimPlayMode::PingPong: {
            float period = s.duration * 2.0f;
            while (s.time >= period) {
                s.time -= period;
                AnimEvent ev{s.entityId, s.clipName, true};
                for (auto& cb : m_impl->callbacks) cb(ev);
            }
            // Reflect if in second half.
            if (s.time > s.duration) s.time = period - s.time;
            break;
        }
        case AnimPlayMode::Hold:
            if (s.time >= s.duration) s.time = s.duration;
            break;
        }
    }
}

const AnimState* AnimationController::GetState(EntityID entity, int layer) const {
    auto it = m_impl->states.find({entity, layer});
    return it != m_impl->states.end() ? &it->second : nullptr;
}

float AnimationController::NormalizedTime(EntityID entity, int layer) const {
    const AnimState* s = GetState(entity, layer);
    if (!s || s->duration <= 0.0f) return 0.0f;
    return s->time / s->duration;
}

void AnimationController::OnAnimEvent(AnimEventCallback cb) {
    m_impl->callbacks.push_back(std::move(cb));
}

void AnimationController::RemoveEntity(EntityID entity) {
    for (auto it = m_impl->states.begin(); it != m_impl->states.end(); ) {
        if (it->first.entity == entity)
            it = m_impl->states.erase(it);
        else
            ++it;
    }
}

void AnimationController::Clear() { m_impl->states.clear(); }

} // namespace Runtime
