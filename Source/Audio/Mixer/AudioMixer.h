#pragma once
#include "Source/Audio/Spatial/SpatialAudio.h"
#include <unordered_map>

namespace NF {

/// @brief Mixes multiple concurrent sounds and manages their playback state.
class AudioMixer {
public:
    /// @brief Begin playback of a sound.
    void PlaySound(SoundId id);

    /// @brief Stop playback of a sound immediately.
    void StopSound(SoundId id);

    /// @brief Set the per-sound volume.
    /// @param id      Sound to adjust.
    /// @param volume  Normalised volume in [0, 1].
    void SetVolume(SoundId id, float volume);

    /// @brief Advance all active sounds by dt seconds.
    void Update(float dt);

private:
    struct SoundState {
        float Volume{1.f};
        float ElapsedTime{0.f};
        bool  Playing{false};
    };

    std::unordered_map<SoundId, SoundState> m_Sounds;
};

} // namespace NF
