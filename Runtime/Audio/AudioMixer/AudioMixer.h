#pragma once
/**
 * @file AudioMixer.h
 * @brief Multi-channel audio mixer with bus routing, per-channel effects, and volume control.
 *
 * AudioMixer manages a hierarchy of audio channels (buses) and mixes audio
 * streams in software:
 *
 *   ChannelType: Master, Music, SFX, Voice, Ambient.
 *   EffectType:  LowPass, HighPass, Reverb, Delay, Distortion, Compressor.
 *
 *   ChannelDesc: type, name, volume (0–1), pan (-1 to +1), muted, soloed,
 *                send list (target channel + send level), effect chain.
 *
 *   AudioMixer:
 *     - AddChannel(desc): register a channel; returns stable ChannelId.
 *     - RemoveChannel(id): remove a channel and its sends.
 *     - SetVolume(id, v): set per-channel volume.
 *     - SetPan(id, pan): set stereo pan.
 *     - SetMute(id, mute) / SetSolo(id, solo).
 *     - AddEffect(id, effect) / RemoveEffect(id, index).
 *     - SetSend(from, to, level): configure a bus send (aux routing).
 *     - MixFrame(dt): advance the mix engine one frame; fires peak callbacks.
 *     - GetPeakLevel(id): post-mix peak in [0,1].
 *     - SetMasterVolume(v) / GetMasterVolume().
 *     - OnPeakReached(cb): called when a channel exceeds the clip threshold.
 *     - MixerStats: channelCount, activeChannels, clippedChannels, mixCallCount.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace Runtime {

// ── Typed handle ──────────────────────────────────────────────────────────
using ChannelId = uint32_t;
static constexpr ChannelId kInvalidChannelId = 0;

// ── Channel type ──────────────────────────────────────────────────────────
enum class ChannelType : uint8_t { Master, Music, SFX, Voice, Ambient };

// ── Effect type ───────────────────────────────────────────────────────────
enum class EffectType : uint8_t { LowPass, HighPass, Reverb, Delay, Distortion, Compressor };

// ── Effect descriptor ─────────────────────────────────────────────────────
struct EffectDesc {
    EffectType type{EffectType::LowPass};
    float      param0{0.5f};   ///< e.g. cutoff freq (normalised)
    float      param1{0.0f};   ///< e.g. resonance / feedback
    float      param2{0.0f};   ///< e.g. wet mix
    bool       enabled{true};
};

// ── Bus send ──────────────────────────────────────────────────────────────
struct BusSend {
    ChannelId target{kInvalidChannelId};
    float     level{1.0f};   ///< Send amount [0, 1]
};

// ── Channel descriptor ────────────────────────────────────────────────────
struct ChannelDesc {
    std::string             name;
    ChannelType             type{ChannelType::SFX};
    float                   volume{1.0f};    ///< [0, 1]
    float                   pan{0.0f};       ///< [-1, +1]
    bool                    muted{false};
    bool                    soloed{false};
    std::vector<BusSend>    sends;           ///< Aux routing
    std::vector<EffectDesc> effects;         ///< Per-channel FX chain
};

// ── Stats ─────────────────────────────────────────────────────────────────
struct MixerStats {
    uint32_t channelCount{0};
    uint32_t activeChannels{0};   ///< Non-muted, non-zero-volume channels
    uint32_t clippedChannels{0};  ///< Channels that peaked above 1.0 this frame
    uint64_t mixCallCount{0};
};

// ── Callbacks ─────────────────────────────────────────────────────────────
using PeakReachedCb = std::function<void(ChannelId, float peakLevel)>;

// ── AudioMixer ────────────────────────────────────────────────────────────
class AudioMixer {
public:
    AudioMixer();
    ~AudioMixer();

    AudioMixer(const AudioMixer&) = delete;
    AudioMixer& operator=(const AudioMixer&) = delete;

    // ── channel management ────────────────────────────────────
    ChannelId  AddChannel(const ChannelDesc& desc);
    bool       RemoveChannel(ChannelId id);
    bool       HasChannel(ChannelId id) const;
    ChannelDesc GetChannelDesc(ChannelId id) const;

    // ── per-channel control ───────────────────────────────────
    void SetVolume(ChannelId id, float volume);
    void SetPan(ChannelId id, float pan);
    void SetMute(ChannelId id, bool mute);
    void SetSolo(ChannelId id, bool solo);

    float GetVolume(ChannelId id) const;
    float GetPan(ChannelId id) const;
    bool  IsMuted(ChannelId id) const;
    bool  IsSoloed(ChannelId id) const;

    // ── effects ───────────────────────────────────────────────
    bool AddEffect(ChannelId id, const EffectDesc& effect);
    bool RemoveEffect(ChannelId id, uint32_t effectIndex);
    bool SetEffectEnabled(ChannelId id, uint32_t effectIndex, bool enabled);
    std::vector<EffectDesc> GetEffects(ChannelId id) const;

    // ── bus routing ───────────────────────────────────────────
    bool SetSend(ChannelId from, ChannelId to, float level);
    bool RemoveSend(ChannelId from, ChannelId to);
    std::vector<BusSend> GetSends(ChannelId from) const;

    // ── master volume ─────────────────────────────────────────
    void  SetMasterVolume(float v);
    float GetMasterVolume() const;

    // ── mix engine ────────────────────────────────────────────
    void  MixFrame(float dt);
    float GetPeakLevel(ChannelId id) const;

    // ── callbacks ─────────────────────────────────────────────
    void OnPeakReached(PeakReachedCb cb);

    // ── queries ───────────────────────────────────────────────
    std::vector<ChannelId> GetAllChannels() const;
    std::vector<ChannelId> GetChannelsByType(ChannelType type) const;
    MixerStats             GetStats() const;

    // ── reset ─────────────────────────────────────────────────
    void Reset();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Runtime
