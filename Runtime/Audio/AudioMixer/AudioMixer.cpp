#include "Runtime/Audio/AudioMixer/AudioMixer.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <stdexcept>

namespace Runtime {

// ── Internal channel state ────────────────────────────────────────────────
struct ChannelState {
    ChannelId   id{kInvalidChannelId};
    ChannelDesc desc;
    float       peakLevel{0.0f};
};

struct AudioMixer::Impl {
    ChannelId                              nextId{1};
    std::unordered_map<ChannelId, ChannelState> channels;
    float                                  masterVolume{1.0f};
    MixerStats                             stats;
    PeakReachedCb                          peakCb;

    static constexpr float kClipThreshold = 1.0f;

    ChannelState* Find(ChannelId id) {
        auto it = channels.find(id);
        return it != channels.end() ? &it->second : nullptr;
    }
    const ChannelState* Find(ChannelId id) const {
        auto it = channels.find(id);
        return it != channels.end() ? &it->second : nullptr;
    }
};

AudioMixer::AudioMixer()  : m_impl(std::make_unique<Impl>()) {}
AudioMixer::~AudioMixer() = default;

// ── Channel management ────────────────────────────────────────────────────
ChannelId AudioMixer::AddChannel(const ChannelDesc& desc) {
    ChannelId id = m_impl->nextId++;
    ChannelState cs;
    cs.id   = id;
    cs.desc = desc;
    m_impl->channels[id] = cs;
    ++m_impl->stats.channelCount;
    return id;
}

bool AudioMixer::RemoveChannel(ChannelId id) {
    auto it = m_impl->channels.find(id);
    if (it == m_impl->channels.end()) return false;
    // Remove any sends pointing to this channel
    for (auto& [cid, cs] : m_impl->channels) {
        auto& sends = cs.desc.sends;
        sends.erase(std::remove_if(sends.begin(), sends.end(),
            [id](const BusSend& s){ return s.target == id; }), sends.end());
    }
    m_impl->channels.erase(it);
    if (m_impl->stats.channelCount > 0) --m_impl->stats.channelCount;
    return true;
}

bool AudioMixer::HasChannel(ChannelId id) const {
    return m_impl->channels.count(id) != 0;
}

ChannelDesc AudioMixer::GetChannelDesc(ChannelId id) const {
    const auto* cs = m_impl->Find(id);
    if (!cs) return {};
    return cs->desc;
}

// ── Per-channel control ───────────────────────────────────────────────────
void AudioMixer::SetVolume(ChannelId id, float v) {
    if (auto* cs = m_impl->Find(id)) cs->desc.volume = std::clamp(v, 0.0f, 1.0f);
}
void AudioMixer::SetPan(ChannelId id, float pan) {
    if (auto* cs = m_impl->Find(id)) cs->desc.pan = std::clamp(pan, -1.0f, 1.0f);
}
void AudioMixer::SetMute(ChannelId id, bool mute) {
    if (auto* cs = m_impl->Find(id)) cs->desc.muted = mute;
}
void AudioMixer::SetSolo(ChannelId id, bool solo) {
    if (auto* cs = m_impl->Find(id)) cs->desc.soloed = solo;
}
float AudioMixer::GetVolume(ChannelId id) const {
    const auto* cs = m_impl->Find(id);
    return cs ? cs->desc.volume : 0.0f;
}
float AudioMixer::GetPan(ChannelId id) const {
    const auto* cs = m_impl->Find(id);
    return cs ? cs->desc.pan : 0.0f;
}
bool AudioMixer::IsMuted(ChannelId id) const {
    const auto* cs = m_impl->Find(id);
    return cs ? cs->desc.muted : false;
}
bool AudioMixer::IsSoloed(ChannelId id) const {
    const auto* cs = m_impl->Find(id);
    return cs ? cs->desc.soloed : false;
}

// ── Effects ───────────────────────────────────────────────────────────────
bool AudioMixer::AddEffect(ChannelId id, const EffectDesc& effect) {
    auto* cs = m_impl->Find(id);
    if (!cs) return false;
    cs->desc.effects.push_back(effect);
    return true;
}
bool AudioMixer::RemoveEffect(ChannelId id, uint32_t effectIndex) {
    auto* cs = m_impl->Find(id);
    if (!cs || effectIndex >= cs->desc.effects.size()) return false;
    cs->desc.effects.erase(cs->desc.effects.begin() + effectIndex);
    return true;
}
bool AudioMixer::SetEffectEnabled(ChannelId id, uint32_t effectIndex, bool enabled) {
    auto* cs = m_impl->Find(id);
    if (!cs || effectIndex >= cs->desc.effects.size()) return false;
    cs->desc.effects[effectIndex].enabled = enabled;
    return true;
}
std::vector<EffectDesc> AudioMixer::GetEffects(ChannelId id) const {
    const auto* cs = m_impl->Find(id);
    return cs ? cs->desc.effects : std::vector<EffectDesc>{};
}

// ── Bus routing ───────────────────────────────────────────────────────────
bool AudioMixer::SetSend(ChannelId from, ChannelId to, float level) {
    auto* cs = m_impl->Find(from);
    if (!cs || !HasChannel(to)) return false;
    for (auto& s : cs->desc.sends) {
        if (s.target == to) { s.level = level; return true; }
    }
    cs->desc.sends.push_back({to, level});
    return true;
}
bool AudioMixer::RemoveSend(ChannelId from, ChannelId to) {
    auto* cs = m_impl->Find(from);
    if (!cs) return false;
    auto& sends = cs->desc.sends;
    auto it = std::remove_if(sends.begin(), sends.end(),
        [to](const BusSend& s){ return s.target == to; });
    if (it == sends.end()) return false;
    sends.erase(it, sends.end());
    return true;
}
std::vector<BusSend> AudioMixer::GetSends(ChannelId from) const {
    const auto* cs = m_impl->Find(from);
    return cs ? cs->desc.sends : std::vector<BusSend>{};
}

// ── Master volume ─────────────────────────────────────────────────────────
void  AudioMixer::SetMasterVolume(float v) { m_impl->masterVolume = std::clamp(v, 0.0f, 1.0f); }
float AudioMixer::GetMasterVolume() const  { return m_impl->masterVolume; }

// ── Mix engine ────────────────────────────────────────────────────────────
void AudioMixer::MixFrame(float /*dt*/) {
    bool hasSolo = false;
    for (auto& [id, cs] : m_impl->channels) { if (cs.desc.soloed) { hasSolo = true; break; } }

    uint32_t active = 0, clipped = 0;
    for (auto& [id, cs] : m_impl->channels) {
        bool audible = !cs.desc.muted && (!hasSolo || cs.desc.soloed) && cs.desc.volume > 0.0f;
        if (!audible) { cs.peakLevel = 0.0f; continue; }
        ++active;

        // Simulate a "peak" based on volume + master (no real audio data here)
        float peak = cs.desc.volume * m_impl->masterVolume;
        // Apply simple effect gain simulation
        for (const auto& fx : cs.desc.effects) {
            if (!fx.enabled) continue;
            if (fx.type == EffectType::Compressor) peak *= (1.0f - fx.param0 * 0.3f);
            if (fx.type == EffectType::Distortion) peak = std::min(peak * (1.0f + fx.param0), 1.2f);
        }
        cs.peakLevel = peak;
        if (peak > Impl::kClipThreshold) {
            ++clipped;
            if (m_impl->peakCb) m_impl->peakCb(id, peak);
        }
    }
    m_impl->stats.activeChannels  = active;
    m_impl->stats.clippedChannels = clipped;
    ++m_impl->stats.mixCallCount;
}

float AudioMixer::GetPeakLevel(ChannelId id) const {
    const auto* cs = m_impl->Find(id);
    return cs ? cs->peakLevel : 0.0f;
}

// ── Callbacks ─────────────────────────────────────────────────────────────
void AudioMixer::OnPeakReached(PeakReachedCb cb) { m_impl->peakCb = std::move(cb); }

// ── Queries ───────────────────────────────────────────────────────────────
std::vector<ChannelId> AudioMixer::GetAllChannels() const {
    std::vector<ChannelId> out;
    for (auto& [id, _] : m_impl->channels) out.push_back(id);
    return out;
}
std::vector<ChannelId> AudioMixer::GetChannelsByType(ChannelType type) const {
    std::vector<ChannelId> out;
    for (auto& [id, cs] : m_impl->channels) if (cs.desc.type == type) out.push_back(id);
    return out;
}
MixerStats AudioMixer::GetStats() const { return m_impl->stats; }

void AudioMixer::Reset() {
    m_impl->channels.clear();
    m_impl->nextId       = 1;
    m_impl->masterVolume = 1.0f;
    m_impl->stats        = {};
}

} // namespace Runtime
