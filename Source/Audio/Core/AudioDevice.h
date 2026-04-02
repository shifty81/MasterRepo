#pragma once
#include <cstdint>

namespace NF {

/// @brief Platform audio backend identifier.
enum class AudioBackend : uint8_t {
    Null    = 0,  ///< Software-only, no actual output.
    WASAPI  = 1,  ///< Windows Audio Session API.
};

/// @brief Manages the audio output device and global volume.
///
/// In Phase 9 we provide a software-mixer path that works on all
/// platforms.  The WASAPI backend is defined but falls back to Null
/// when unavailable (e.g. on Linux CI).
///
/// Call Init(), then UpdateMix() each frame, then Shutdown().
class AudioDevice {
public:
    /// @brief Initialise the audio device.
    /// @param backend  Requested backend (falls back to Null if unavailable).
    /// @return True on success.
    bool Init(AudioBackend backend = AudioBackend::Null);

    /// @brief Release all audio device resources.
    void Shutdown();

    /// @brief True after a successful Init().
    [[nodiscard]] bool IsInitialized() const noexcept { return m_Initialised; }

    /// @brief The active backend.
    [[nodiscard]] AudioBackend GetBackend() const noexcept { return m_Backend; }

    /// @brief Set the master output volume.
    /// @param volume  Normalised value in [0, 1].
    void SetMasterVolume(float volume) noexcept;

    /// @brief Return the current master volume in [0, 1].
    [[nodiscard]] float GetMasterVolume() const noexcept;

    /// @brief Sample rate of the output device (default 44100).
    [[nodiscard]] uint32_t GetSampleRate() const noexcept { return m_SampleRate; }

    /// @brief Number of output channels (default 2 = stereo).
    [[nodiscard]] uint32_t GetChannels() const noexcept { return m_Channels; }

private:
    float        m_MasterVolume{1.f};
    bool         m_Initialised{false};
    AudioBackend m_Backend{AudioBackend::Null};
    uint32_t     m_SampleRate{44100};
    uint32_t     m_Channels{2};
};

} // namespace NF
