#pragma once
#include "Audio/Core/SoundAsset.h"
#include "Audio/Mixer/AudioMixer.h"
#include "Core/Math/Vector.h"
#include <cstdint>

namespace NF {

/// @brief Configuration for 3-D sound attenuation.
struct SpatialConfig {
    float MinDistance{1.f};   ///< Distance at which volume starts to attenuate.
    float MaxDistance{50.f};  ///< Distance beyond which volume is zero.
    float Rolloff{1.f};      ///< Rolloff factor (1 = inverse distance).
};

/// @brief Manages 3-D audio playback and listener state.
///
/// Uses inverse-distance attenuation and simple stereo panning based
/// on the relative angle between the listener forward direction and
/// the sound source.
class SpatialAudio {
public:
    /// @brief Set the @c AudioMixer to route channels through.
    void SetMixer(AudioMixer* mixer) noexcept { m_Mixer = mixer; }

    // ---- Listener -----------------------------------------------------------

    /// @brief Set the world-space position of the audio listener.
    void SetListenerPosition(const Vector3& position) noexcept;

    /// @brief Set the orientation of the audio listener.
    /// @param forward  Unit-length look direction.
    /// @param up       Unit-length up vector.
    void SetListenerOrientation(const Vector3& forward, const Vector3& up) noexcept;

    /// @brief Current listener position.
    [[nodiscard]] const Vector3& GetListenerPosition() const noexcept { return m_ListenerPos; }
    [[nodiscard]] const Vector3& GetListenerForward()  const noexcept { return m_ListenerForward; }
    [[nodiscard]] const Vector3& GetListenerUp()       const noexcept { return m_ListenerUp; }

    // ---- 3-D playback -------------------------------------------------------

    /// @brief Play a sound at a world-space position with spatial attenuation.
    /// @param soundId   Identifier of the sound to play.
    /// @param position  World-space emission point.
    /// @param volume    Base volume in [0, 1] (before attenuation).
    /// @param config    Attenuation configuration.
    /// @return ChannelHandle from the mixer, or 0 on failure.
    ChannelHandle PlayAt(SoundId soundId, const Vector3& position,
                         float volume = 1.f,
                         const SpatialConfig& config = {});

    // ---- Attenuation helpers ------------------------------------------------

    /// @brief Compute the attenuation factor for a source at @p distance.
    ///
    /// Uses inverse-distance clamped model:
    ///   factor = clamp(MinDistance / (MinDistance + Rolloff * (dist - MinDistance)), 0, 1)
    [[nodiscard]] static float ComputeAttenuation(float distance,
                                                   const SpatialConfig& config) noexcept;

    /// @brief Compute stereo pan [-1, +1] based on listener-to-source direction.
    ///
    /// Returns the dot product between the listener's right vector (derived from
    /// forward × up) and the normalized direction to the source.  Positive = right,
    /// negative = left.
    [[nodiscard]] float ComputePan(const Vector3& sourcePos) const noexcept;

private:
    AudioMixer* m_Mixer{nullptr};
    Vector3 m_ListenerPos{};
    Vector3 m_ListenerForward{0.f, 0.f, -1.f};
    Vector3 m_ListenerUp{0.f, 1.f, 0.f};
};

} // namespace NF
