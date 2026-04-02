#include "Audio/Core/SoundBank.h"
#include "Core/Logging/Log.h"
#include <cmath>
#include <fstream>
#include <cstring>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace NF {

// ============================================================================
// WAV loading
// ============================================================================

namespace {

#pragma pack(push, 1)
struct WavHeader {
    char     riffTag[4];      // "RIFF"
    uint32_t riffSize;
    char     waveTag[4];      // "WAVE"
};
struct WavChunkHeader {
    char     id[4];
    uint32_t size;
};
struct WavFmt {
    uint16_t format;          // 1 = PCM, 3 = IEEE float
    uint16_t channels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
};
#pragma pack(pop)

} // anonymous

SoundId SoundBank::LoadWav(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        Logger::Log(LogLevel::Warning, "Audio",
                    "SoundBank::LoadWav — cannot open: " + filePath);
        return 0;
    }

    // Read RIFF header
    WavHeader hdr{};
    file.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (std::memcmp(hdr.riffTag, "RIFF", 4) != 0 ||
        std::memcmp(hdr.waveTag, "WAVE", 4) != 0) {
        Logger::Log(LogLevel::Warning, "Audio",
                    "SoundBank::LoadWav — not a valid WAV: " + filePath);
        return 0;
    }

    // Scan for "fmt " and "data" chunks
    WavFmt fmt{};
    bool fmtFound = false;
    std::vector<uint8_t> rawData;

    while (file) {
        WavChunkHeader ch{};
        file.read(reinterpret_cast<char*>(&ch), sizeof(ch));
        if (!file) break;

        if (std::memcmp(ch.id, "fmt ", 4) == 0) {
            const size_t toRead = std::min<size_t>(ch.size, sizeof(fmt));
            file.read(reinterpret_cast<char*>(&fmt), static_cast<std::streamsize>(toRead));
            if (ch.size > toRead)
                file.seekg(static_cast<std::streamoff>(ch.size - toRead), std::ios::cur);
            fmtFound = true;
        } else if (std::memcmp(ch.id, "data", 4) == 0) {
            rawData.resize(ch.size);
            file.read(reinterpret_cast<char*>(rawData.data()),
                       static_cast<std::streamsize>(ch.size));
        } else {
            // Skip unknown chunk (pad to word boundary).
            uint32_t skip = ch.size + (ch.size & 1u);
            file.seekg(static_cast<std::streamoff>(skip), std::ios::cur);
            if (!file.good()) break;
        }
    }

    if (!fmtFound || rawData.empty()) {
        Logger::Log(LogLevel::Warning, "Audio",
                    "SoundBank::LoadWav — missing fmt/data in: " + filePath);
        return 0;
    }

    if (fmt.format != 1 /* PCM */ && fmt.format != 3 /* IEEE float */) {
        Logger::Log(LogLevel::Warning, "Audio",
                    "SoundBank::LoadWav — unsupported format code " +
                    std::to_string(fmt.format) + " in: " + filePath);
        return 0;
    }

    // Convert to float samples
    SoundAsset asset;
    asset.SampleRate = fmt.sampleRate;
    asset.Channels   = fmt.channels;

    if (fmt.format == 1 && fmt.bitsPerSample == 16) {
        const size_t sampleCount = rawData.size() / 2;
        asset.Samples.resize(sampleCount);
        const auto* src = reinterpret_cast<const int16_t*>(rawData.data());
        for (size_t i = 0; i < sampleCount; ++i)
            asset.Samples[i] = static_cast<float>(src[i]) / 32768.f;
    } else if (fmt.format == 3 && fmt.bitsPerSample == 32) {
        const size_t sampleCount = rawData.size() / 4;
        asset.Samples.resize(sampleCount);
        std::memcpy(asset.Samples.data(), rawData.data(), rawData.size());
    } else if (fmt.format == 1 && fmt.bitsPerSample == 8) {
        const size_t sampleCount = rawData.size();
        asset.Samples.resize(sampleCount);
        for (size_t i = 0; i < sampleCount; ++i)
            asset.Samples[i] = (static_cast<float>(rawData[i]) - 128.f) / 128.f;
    } else {
        Logger::Log(LogLevel::Warning, "Audio",
                    "SoundBank::LoadWav — unsupported PCM bit-depth " +
                    std::to_string(fmt.bitsPerSample) + " in: " + filePath);
        return 0;
    }

    asset.Duration = asset.FrameCount() > 0
        ? static_cast<float>(asset.FrameCount()) / static_cast<float>(asset.SampleRate)
        : 0.f;

    // Extract file name as asset name
    size_t slash = filePath.find_last_of("/\\");
    asset.Name = (slash != std::string::npos)
                     ? filePath.substr(slash + 1)
                     : filePath;

    const SoundId id = m_NextId++;
    asset.Id = id;
    m_NameIndex[asset.Name] = id;
    m_Assets.emplace(id, std::move(asset));

    Logger::Log(LogLevel::Info, "Audio",
                "Loaded WAV: " + filePath +
                " (id=" + std::to_string(id) +
                " rate=" + std::to_string(fmt.sampleRate) +
                " ch=" + std::to_string(fmt.channels) +
                " frames=" + std::to_string(m_Assets[id].FrameCount()) + ")");
    return id;
}

// ============================================================================
// Procedural generation
// ============================================================================

SoundId SoundBank::GenerateTone(const std::string& name,
                                 float frequency, float duration, float volume)
{
    constexpr uint32_t kRate = 44100;
    const uint32_t frameCount = static_cast<uint32_t>(duration * static_cast<float>(kRate));

    SoundAsset asset;
    asset.SampleRate = kRate;
    asset.Channels   = 1;
    asset.Name       = name;
    asset.Samples.resize(frameCount);

    for (uint32_t i = 0; i < frameCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kRate);
        asset.Samples[i] = volume * std::sin(2.f * static_cast<float>(M_PI) * frequency * t);
    }
    asset.Duration = duration;

    const SoundId id = m_NextId++;
    asset.Id = id;
    m_NameIndex[name] = id;
    m_Assets.emplace(id, std::move(asset));
    return id;
}

SoundId SoundBank::GenerateNoise(const std::string& name,
                                  float duration, float volume)
{
    constexpr uint32_t kRate = 44100;
    const uint32_t frameCount = static_cast<uint32_t>(duration * static_cast<float>(kRate));

    SoundAsset asset;
    asset.SampleRate = kRate;
    asset.Channels   = 1;
    asset.Name       = name;
    asset.Samples.resize(frameCount);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.f, 1.f);
    for (uint32_t i = 0; i < frameCount; ++i)
        asset.Samples[i] = volume * dist(rng);
    asset.Duration = duration;

    const SoundId id = m_NextId++;
    asset.Id = id;
    m_NameIndex[name] = id;
    m_Assets.emplace(id, std::move(asset));
    return id;
}

// ============================================================================
// Queries
// ============================================================================

const SoundAsset* SoundBank::Get(SoundId id) const noexcept {
    auto it = m_Assets.find(id);
    return it != m_Assets.end() ? &it->second : nullptr;
}

const SoundAsset* SoundBank::GetByName(const std::string& name) const noexcept {
    auto it = m_NameIndex.find(name);
    if (it == m_NameIndex.end()) return nullptr;
    return Get(it->second);
}

void SoundBank::Clear() noexcept {
    m_Assets.clear();
    m_NameIndex.clear();
}

} // namespace NF
