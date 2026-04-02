#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace NF {

/// @brief Opaque identifier for a loaded sound asset.
using SoundId = uint32_t;

/// @brief In-memory representation of a loaded sound.
///
/// Stores interleaved PCM float samples.  Phase 9 supports WAV
/// (uncompressed PCM 16-bit or float) loading.
struct SoundAsset {
    SoundId              Id{0};
    std::string          Name;
    uint32_t             SampleRate{44100};
    uint32_t             Channels{1};        ///< 1 = mono, 2 = stereo
    std::vector<float>   Samples;            ///< Interleaved PCM floats in [-1, 1].
    float                Duration{0.f};      ///< Duration in seconds.

    /// @brief True when valid samples have been loaded.
    [[nodiscard]] bool IsValid() const noexcept {
        return !Samples.empty() && SampleRate > 0 && Channels > 0;
    }

    /// @brief Total number of sample frames (samples / channels).
    [[nodiscard]] uint32_t FrameCount() const noexcept {
        return Channels > 0 ? static_cast<uint32_t>(Samples.size()) / Channels : 0;
    }
};

} // namespace NF
