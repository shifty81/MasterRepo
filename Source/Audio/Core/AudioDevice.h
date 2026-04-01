#pragma once

namespace NF {

/// @brief Manages the audio output device and global volume.
class AudioDevice {
public:
    /// @brief Initialise the audio device.
    /// @return True on success.
    bool Init();

    /// @brief Release all audio device resources.
    void Shutdown();

    /// @brief Set the master output volume.
    /// @param volume  Normalised value in [0, 1].
    void SetMasterVolume(float volume) noexcept;

    /// @brief Return the current master volume in [0, 1].
    [[nodiscard]] float GetMasterVolume() const noexcept;

private:
    float m_MasterVolume{1.f};
    bool  m_Initialised{false};
};

} // namespace NF
