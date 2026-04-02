#include "Audio/Mixer/AudioMixer.h"
#include <algorithm>
#include <cmath>

namespace NF {

ChannelHandle AudioMixer::Play(SoundId soundId, float volume, float pan,
                                bool loop, uint8_t priority)
{
    // Enforce soft channel limit — evict lowest-priority stopped channel.
    if (static_cast<int>(m_Channels.size()) >= kMaxChannels) {
        // Try to remove a non-playing channel first.
        ChannelHandle victimHandle = 0;
        uint8_t       worstPriority = 0;
        for (auto& [h, ch] : m_Channels) {
            if (!ch.Playing && ch.Priority >= worstPriority) {
                worstPriority = ch.Priority;
                victimHandle  = h;
            }
        }
        if (victimHandle != 0) {
            m_Channels.erase(victimHandle);
        } else {
            // All channels playing — evict lowest-priority one.
            for (auto& [h, ch] : m_Channels) {
                if (ch.Priority >= worstPriority) {
                    worstPriority = ch.Priority;
                    victimHandle  = h;
                }
            }
            if (victimHandle != 0)
                m_Channels.erase(victimHandle);
        }
    }

    const ChannelHandle handle = m_NextHandle++;
    MixerChannel ch;
    ch.SoundID     = soundId;
    ch.Volume      = std::clamp(volume, 0.f, 1.f);
    ch.Pan         = std::clamp(pan, -1.f, 1.f);
    ch.PlaybackPos = 0.f;
    ch.Playing     = true;
    ch.Looping     = loop;
    ch.Priority    = priority;
    m_Channels[handle] = ch;
    return handle;
}

void AudioMixer::Stop(ChannelHandle handle)
{
    if (auto it = m_Channels.find(handle); it != m_Channels.end())
        it->second.Playing = false;
}

void AudioMixer::StopAll()
{
    for (auto& [h, ch] : m_Channels)
        ch.Playing = false;
}

void AudioMixer::SetChannelVolume(ChannelHandle handle, float volume)
{
    if (auto it = m_Channels.find(handle); it != m_Channels.end())
        it->second.Volume = std::clamp(volume, 0.f, 1.f);
}

void AudioMixer::SetChannelPan(ChannelHandle handle, float pan)
{
    if (auto it = m_Channels.find(handle); it != m_Channels.end())
        it->second.Pan = std::clamp(pan, -1.f, 1.f);
}

void AudioMixer::Update(float dt)
{
    std::vector<ChannelHandle> expired;

    for (auto& [handle, ch] : m_Channels) {
        if (!ch.Playing) continue;

        ch.PlaybackPos += dt;

        // Check if playback has exceeded the asset duration.
        float duration = 0.f;
        if (m_Bank) {
            if (const SoundAsset* asset = m_Bank->Get(ch.SoundID))
                duration = asset->Duration;
        }

        if (duration > 0.f && ch.PlaybackPos >= duration) {
            if (ch.Looping) {
                // Wrap around using fmod for efficiency.
                ch.PlaybackPos = std::fmod(ch.PlaybackPos, duration);
            } else {
                ch.Playing = false;
                expired.push_back(handle);
            }
        }
    }

    // Remove expired channels.
    for (auto h : expired)
        m_Channels.erase(h);
}

int AudioMixer::ActiveChannelCount() const noexcept
{
    int count = 0;
    for (auto& [h, ch] : m_Channels)
        if (ch.Playing) ++count;
    return count;
}

const MixerChannel* AudioMixer::GetChannel(ChannelHandle handle) const noexcept
{
    auto it = m_Channels.find(handle);
    return it != m_Channels.end() ? &it->second : nullptr;
}

} // namespace NF
