#pragma once
#include "Audio/Core/SoundAsset.h"
#include "Audio/Core/SoundBank.h"
#include <unordered_map>
#include <vector>

namespace NF {

/// @brief Per-channel playback state tracked by the mixer.
struct MixerChannel {
    SoundId  SoundID{0};       ///< Which sound asset is playing.
    float    Volume{1.f};      ///< Per-channel volume [0, 1].
    float    Pan{0.f};         ///< Stereo pan [-1 = full left, 0 = center, +1 = full right].
    float    PlaybackPos{0.f}; ///< Current playback position in seconds.
    bool     Playing{false};   ///< True while actively playing.
    bool     Looping{false};   ///< True to restart at end.
    uint8_t  Priority{128};    ///< 0 = highest, 255 = lowest.
};

/// @brief Handle identifying a playing channel in the mixer.
using ChannelHandle = uint32_t;

/// @brief Mixes multiple concurrent sounds and manages their playback state.
///
/// Each call to @c Play() allocates a @c MixerChannel with a unique handle.
/// @c Update() advances all active channels; expired non-looping channels are
/// automatically released.
class AudioMixer {
public:
    AudioMixer() = default;

    /// @brief Set the non-owning pointer to the sound bank.
    void SetSoundBank(const SoundBank* bank) noexcept { m_Bank = bank; }

    // ---- Playback -----------------------------------------------------------

    /// @brief Start playback of a sound.
    /// @param soundId  Asset identifier from @c SoundBank.
    /// @param volume   Per-channel volume [0, 1].
    /// @param pan      Stereo pan [-1, +1].
    /// @param loop     True to loop.
    /// @param priority Channel priority (lower = more important).
    /// @return Handle to the playing channel, or 0 on failure.
    ChannelHandle Play(SoundId soundId,
                       float volume   = 1.f,
                       float pan      = 0.f,
                       bool  loop     = false,
                       uint8_t priority = 128);

    /// @brief Stop playback on a specific channel immediately.
    void Stop(ChannelHandle handle);

    /// @brief Stop all active channels.
    void StopAll();

    /// @brief Set volume on a playing channel.
    void SetChannelVolume(ChannelHandle handle, float volume);

    /// @brief Set pan on a playing channel.
    void SetChannelPan(ChannelHandle handle, float pan);

    // ---- Per-frame ----------------------------------------------------------

    /// @brief Advance all active channels by @p dt seconds.
    ///
    /// Channels that exceed their asset's duration are either looped or
    /// automatically removed.
    void Update(float dt);

    // ---- Queries ------------------------------------------------------------

    /// @brief Number of currently active (playing) channels.
    [[nodiscard]] int ActiveChannelCount() const noexcept;

    /// @brief Total channels allocated (including stopped).
    [[nodiscard]] int TotalChannelCount() const noexcept {
        return static_cast<int>(m_Channels.size());
    }

    /// @brief Retrieve a channel state, or nullptr.
    [[nodiscard]] const MixerChannel* GetChannel(ChannelHandle handle) const noexcept;

    /// @brief Maximum concurrent channels (soft limit).
    static constexpr int kMaxChannels = 64;

private:
    const SoundBank*   m_Bank{nullptr};
    ChannelHandle      m_NextHandle{1};
    std::unordered_map<ChannelHandle, MixerChannel> m_Channels;
};

} // namespace NF
