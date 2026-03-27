#include "Engine/Audio/Cue/SoundCue/SoundCue.h"
#include <algorithm>
#include <random>
#include <unordered_map>

namespace Engine {

struct CueState {
    CueDef def;
    std::vector<CueSample> samples;
    float cooldownRemaining{0};
    std::string lastClip;
    int seqIndex{0};
    std::vector<uint32_t> shuffleOrder;
    int shufflePos{0};
    std::function<void(uint32_t)> onPlay, onStop;
};

struct SoundInstance {
    uint32_t cueId;
    std::string clip;
    bool playing{true};
};

struct SoundCue::Impl {
    std::unordered_map<uint32_t, CueState>    cues;
    std::unordered_map<uint32_t, SoundInstance> instances;
    uint32_t nextInst{1};
    std::mt19937 rng{42};
};

SoundCue::SoundCue()  { m_impl = new Impl; }
SoundCue::~SoundCue() { delete m_impl; }
void SoundCue::Init    () {}
void SoundCue::Shutdown() { Reset(); }
void SoundCue::Reset   () { m_impl->cues.clear(); m_impl->instances.clear(); }

bool SoundCue::RegisterCue(const CueDef& def) {
    if (m_impl->cues.count(def.id)) return false;
    CueState s; s.def = def;
    m_impl->cues[def.id] = std::move(s);
    return true;
}
void SoundCue::AddSample(uint32_t cueId, const CueSample& sample) {
    auto it = m_impl->cues.find(cueId);
    if (it != m_impl->cues.end()) it->second.samples.push_back(sample);
}
void SoundCue::RemoveSample(uint32_t cueId, const std::string& clip) {
    auto it = m_impl->cues.find(cueId);
    if (it == m_impl->cues.end()) return;
    auto& s = it->second.samples;
    s.erase(std::remove_if(s.begin(), s.end(),
        [&](const CueSample& cs){ return cs.clip == clip; }), s.end());
}

uint32_t SoundCue::Play(uint32_t cueId, float, float, float) {
    auto it = m_impl->cues.find(cueId);
    if (it == m_impl->cues.end() || it->second.samples.empty()) return 0;
    if (it->second.cooldownRemaining > 0) return 0;
    CueState& cs = it->second;
    CueSample* chosen = nullptr;
    if (cs.def.playMode == CuePlayMode::Sequential) {
        chosen = &cs.samples[cs.seqIndex % cs.samples.size()];
        ++cs.seqIndex;
    } else {
        // weighted random
        float total = 0; for (auto& s : cs.samples) total += s.weight;
        std::uniform_real_distribution<float> d(0, total);
        float r = d(m_impl->rng);
        for (auto& s : cs.samples) { r -= s.weight; if (r <= 0) { chosen = &s; break; } }
        if (!chosen) chosen = &cs.samples.back();
    }
    cs.lastClip = chosen->clip;
    cs.cooldownRemaining = cs.def.cooldown;
    uint32_t id = m_impl->nextInst++;
    m_impl->instances[id] = {cueId, chosen->clip, true};
    if (cs.onPlay) cs.onPlay(id);
    return id;
}
void SoundCue::Stop(uint32_t instId) {
    auto it = m_impl->instances.find(instId);
    if (it == m_impl->instances.end()) return;
    it->second.playing = false;
    auto cit = m_impl->cues.find(it->second.cueId);
    if (cit != m_impl->cues.end() && cit->second.onStop) cit->second.onStop(instId);
}
bool SoundCue::IsPlaying(uint32_t instId) const {
    auto it = m_impl->instances.find(instId);
    return it != m_impl->instances.end() && it->second.playing;
}
void SoundCue::Tick(float dt) {
    for (auto& [id, cs] : m_impl->cues)
        if (cs.cooldownRemaining > 0) cs.cooldownRemaining -= dt;
}
void SoundCue::SetCooldown(uint32_t cueId, float secs) {
    auto it = m_impl->cues.find(cueId); if (it != m_impl->cues.end()) it->second.def.cooldown = secs;
}
std::string SoundCue::GetLastPlayedClip(uint32_t cueId) const {
    auto it = m_impl->cues.find(cueId); return it != m_impl->cues.end() ? it->second.lastClip : "";
}
uint32_t SoundCue::GetSampleCount(uint32_t cueId) const {
    auto it = m_impl->cues.find(cueId); return it != m_impl->cues.end() ? (uint32_t)it->second.samples.size() : 0;
}
void SoundCue::SetOnPlay(uint32_t cueId, std::function<void(uint32_t)> cb) {
    auto it = m_impl->cues.find(cueId); if (it != m_impl->cues.end()) it->second.onPlay = std::move(cb);
}
void SoundCue::SetOnStop(uint32_t cueId, std::function<void(uint32_t)> cb) {
    auto it = m_impl->cues.find(cueId); if (it != m_impl->cues.end()) it->second.onStop = std::move(cb);
}

} // namespace Engine
