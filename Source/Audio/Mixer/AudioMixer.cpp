#include "Audio/Mixer/AudioMixer.h"
#include <algorithm>

namespace NF {

void AudioMixer::PlaySound(SoundId id) {
    auto& s = m_Sounds[id];
    s.Playing     = true;
    s.ElapsedTime = 0.f;
}

void AudioMixer::StopSound(SoundId id) {
    if (auto it = m_Sounds.find(id); it != m_Sounds.end())
        it->second.Playing = false;
}

void AudioMixer::SetVolume(SoundId id, float volume) {
    m_Sounds[id].Volume = std::clamp(volume, 0.f, 1.f);
}

void AudioMixer::Update(float dt) {
    for (auto& [id, state] : m_Sounds) {
        if (state.Playing)
            state.ElapsedTime += dt;
    }
}

} // namespace NF
