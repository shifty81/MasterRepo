#pragma once
#include "Audio/Core/SoundAsset.h"
#include <unordered_map>
#include <string>

namespace NF {

/// @brief Owns all loaded sound assets keyed by @c SoundId.
///
/// Provides WAV loading (16-bit PCM, mono or stereo) and procedural
/// tone generation for placeholder sounds.
class SoundBank {
public:
    SoundBank() = default;

    // ---- Asset loading ------------------------------------------------------

    /// @brief Load a WAV file from disk and return its id.
    /// @return 0 on failure.
    SoundId LoadWav(const std::string& filePath);

    /// @brief Generate a sine-wave tone for placeholder use.
    /// @param name      Friendly name for the asset.
    /// @param frequency Tone frequency in Hz.
    /// @param duration  Duration in seconds.
    /// @param volume    Peak amplitude [0, 1].
    /// @return SoundId of the generated asset.
    SoundId GenerateTone(const std::string& name,
                         float frequency, float duration, float volume = 0.5f);

    /// @brief Generate a noise burst for placeholder use.
    SoundId GenerateNoise(const std::string& name,
                          float duration, float volume = 0.3f);

    // ---- Queries ------------------------------------------------------------

    /// @brief Retrieve a loaded asset by id, or nullptr.
    [[nodiscard]] const SoundAsset* Get(SoundId id) const noexcept;

    /// @brief Retrieve a loaded asset by name, or nullptr.
    [[nodiscard]] const SoundAsset* GetByName(const std::string& name) const noexcept;

    /// @brief Number of loaded assets.
    [[nodiscard]] size_t Count() const noexcept { return m_Assets.size(); }

    /// @brief Remove all loaded assets.
    void Clear() noexcept;

private:
    SoundId m_NextId{1};
    std::unordered_map<SoundId, SoundAsset> m_Assets;
    std::unordered_map<std::string, SoundId> m_NameIndex;
};

} // namespace NF
