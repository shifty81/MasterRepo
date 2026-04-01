#pragma once
#include "Source/Core/Math/Vector.h"
#include <cstdint>

namespace NF {

/// @brief Opaque identifier for a loaded sound asset.
using SoundId = uint32_t;

/// @brief Manages 3-D audio playback and listener state.
class SpatialAudio {
public:
    /// @brief Set the world-space position of the audio listener.
    void SetListenerPosition(const Vector3& position) noexcept;

    /// @brief Set the orientation of the audio listener.
    /// @param forward  Unit-length look direction.
    /// @param up       Unit-length up vector.
    void SetListenerOrientation(const Vector3& forward, const Vector3& up) noexcept;

    /// @brief Play a sound at a world-space position.
    /// @param soundId   Identifier of the sound to play.
    /// @param position  World-space emission point.
    /// @param volume    Normalised volume in [0, 1].
    void PlayAt(SoundId soundId, const Vector3& position, float volume);

    /// @brief Current listener position.
    [[nodiscard]] const Vector3& GetListenerPosition() const noexcept { return m_ListenerPos; }

private:
    Vector3 m_ListenerPos{};
    Vector3 m_ListenerForward{0.f, 0.f, -1.f};
    Vector3 m_ListenerUp{0.f, 1.f, 0.f};
};

} // namespace NF
