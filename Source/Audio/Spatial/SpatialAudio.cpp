#include "Audio/Spatial/SpatialAudio.h"
#include <cmath>
#include <algorithm>

namespace NF {

void SpatialAudio::SetListenerPosition(const Vector3& position) noexcept {
    m_ListenerPos = position;
}

void SpatialAudio::SetListenerOrientation(const Vector3& forward,
                                           const Vector3& up) noexcept {
    m_ListenerForward = forward;
    m_ListenerUp      = up;
}

ChannelHandle SpatialAudio::PlayAt(SoundId soundId, const Vector3& position,
                                    float volume, const SpatialConfig& config)
{
    if (!m_Mixer) return 0;

    const Vector3 diff = position - m_ListenerPos;
    const float dist = diff.Length();

    const float attenuation = ComputeAttenuation(dist, config);
    const float finalVolume = std::clamp(volume * attenuation, 0.f, 1.f);

    const float pan = ComputePan(position);

    return m_Mixer->Play(soundId, finalVolume, pan);
}

float SpatialAudio::ComputeAttenuation(float distance,
                                        const SpatialConfig& config) noexcept
{
    if (distance <= config.MinDistance)
        return 1.f;
    if (distance >= config.MaxDistance)
        return 0.f;

    // Inverse-distance clamped model.
    const float denom = config.MinDistance +
        config.Rolloff * (distance - config.MinDistance);
    if (denom <= 0.f) return 0.f;

    return std::clamp(config.MinDistance / denom, 0.f, 1.f);
}

float SpatialAudio::ComputePan(const Vector3& sourcePos) const noexcept
{
    const Vector3 diff = sourcePos - m_ListenerPos;
    const float dist = diff.Length();
    if (dist < 1e-6f) return 0.f;  // On top of listener — center.

    // Derive listener right vector from forward × up.
    const Vector3 right = m_ListenerForward.Cross(m_ListenerUp).Normalized();

    // Direction to the source.
    const Vector3 dir = diff / dist;

    // Dot with right → positive = right ear, negative = left ear.
    return std::clamp(right.Dot(dir), -1.f, 1.f);
}

} // namespace NF
