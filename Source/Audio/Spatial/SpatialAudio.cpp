#include "Audio/Spatial/SpatialAudio.h"

namespace NF {

void SpatialAudio::SetListenerPosition(const Vector3& position) noexcept {
    m_ListenerPos = position;
}

void SpatialAudio::SetListenerOrientation(const Vector3& forward, const Vector3& up) noexcept {
    m_ListenerForward = forward;
    m_ListenerUp      = up;
}

void SpatialAudio::PlayAt(SoundId /*soundId*/, const Vector3& /*position*/, float /*volume*/) {
    // Stub: integrate with platform audio API to spatialise and play the sound.
}

} // namespace NF
