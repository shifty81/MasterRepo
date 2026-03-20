#include "PCG/Audio/AudioGenerator.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <random>

namespace PCG {

// ──────────────────────────────────────────────────────────────
// AudioClip helpers
// ──────────────────────────────────────────────────────────────

bool AudioClip::SaveWAV(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    AudioGenerator::WriteWAVHeader(f, sampleRate, (int32_t)samples.size());
    f.write(reinterpret_cast<const char*>(samples.data()),
            (std::streamsize)(samples.size() * sizeof(int16_t)));
    return true;
}

// ──────────────────────────────────────────────────────────────
// Private helpers
// ──────────────────────────────────────────────────────────────

int16_t AudioGenerator::FloatToInt16(float v) {
    v = std::max(-1.f, std::min(1.f, v));
    return static_cast<int16_t>(v * 32767.f);
}

bool AudioGenerator::WriteWAVHeader(std::ostream& out, int32_t sampleRate,
                                     int32_t numSamples, int32_t bitsPerSample,
                                     int32_t numChannels) {
    int32_t byteRate   = sampleRate * numChannels * bitsPerSample / 8;
    int16_t blockAlign = (int16_t)(numChannels * bitsPerSample / 8);
    int32_t dataSize   = numSamples * numChannels * bitsPerSample / 8;
    int32_t fileSize   = 36 + dataSize;

    out.write("RIFF", 4);
    out.write(reinterpret_cast<const char*>(&fileSize), 4);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    int32_t fmtSize = 16;
    out.write(reinterpret_cast<const char*>(&fmtSize), 4);
    int16_t audioFmt = 1;
    out.write(reinterpret_cast<const char*>(&audioFmt), 2);
    int16_t ch = (int16_t)numChannels;
    out.write(reinterpret_cast<const char*>(&ch), 2);
    out.write(reinterpret_cast<const char*>(&sampleRate), 4);
    out.write(reinterpret_cast<const char*>(&byteRate), 4);
    out.write(reinterpret_cast<const char*>(&blockAlign), 2);
    int16_t bps = (int16_t)bitsPerSample;
    out.write(reinterpret_cast<const char*>(&bps), 2);
    out.write("data", 4);
    out.write(reinterpret_cast<const char*>(&dataSize), 4);
    return true;
}

// ──────────────────────────────────────────────────────────────
// Generators
// ──────────────────────────────────────────────────────────────

AudioClip AudioGenerator::GenerateTone(float frequencyHz, float durationSec,
                                        int32_t sampleRate, float amplitude) {
    int32_t n = (int32_t)(sampleRate * durationSec);
    AudioClip clip; clip.sampleRate = sampleRate; clip.samples.resize((size_t)n);
    const float twoPi = 2.f * 3.14159265359f;
    for (int32_t i = 0; i < n; ++i) {
        float t = (float)i / sampleRate;
        clip.samples[i] = FloatToInt16(amplitude * std::sin(twoPi * frequencyHz * t));
    }
    return clip;
}

AudioClip AudioGenerator::GenerateMultiTone(const std::vector<float>& frequencies,
                                             const std::vector<float>& amplitudes,
                                             float durationSec, int32_t sampleRate) {
    if (frequencies.empty() || frequencies.size() != amplitudes.size())
        return {};
    int32_t n = (int32_t)(sampleRate * durationSec);
    AudioClip clip; clip.sampleRate = sampleRate; clip.samples.resize((size_t)n);
    const float twoPi = 2.f * 3.14159265359f;
    for (int32_t i = 0; i < n; ++i) {
        float t = (float)i / sampleRate;
        float v = 0.f;
        for (size_t j = 0; j < frequencies.size(); ++j)
            v += amplitudes[j] * std::sin(twoPi * frequencies[j] * t);
        v /= (float)frequencies.size();
        clip.samples[i] = FloatToInt16(v);
    }
    return clip;
}

AudioClip AudioGenerator::GenerateExplosion(float durationSec, int32_t sampleRate,
                                             uint64_t seed) {
    int32_t n = (int32_t)(sampleRate * durationSec);
    AudioClip clip; clip.sampleRate = sampleRate; clip.samples.resize((size_t)n);
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<float> dis(-1.f, 1.f);
    const float twoPi = 2.f * 3.14159265359f;
    for (int32_t i = 0; i < n; ++i) {
        float t   = (float)i / n;
        float env = std::exp(-5.f * t);
        float v   = env * (0.7f * dis(rng) + 0.3f * std::sin(twoPi * 80.f * t));
        clip.samples[i] = FloatToInt16(v);
    }
    return clip;
}

AudioClip AudioGenerator::GenerateLaser(float startFreqHz, float endFreqHz,
                                         float durationSec, int32_t sampleRate) {
    int32_t n = (int32_t)(sampleRate * durationSec);
    AudioClip clip; clip.sampleRate = sampleRate; clip.samples.resize((size_t)n);
    const float twoPi = 2.f * 3.14159265359f;
    for (int32_t i = 0; i < n; ++i) {
        float t    = (float)i / n;
        float freq = startFreqHz + (endFreqHz - startFreqHz) * t;
        float env  = std::exp(-3.f * t);
        float v    = env * std::sin(twoPi * freq * (float)i / sampleRate);
        clip.samples[i] = FloatToInt16(v);
    }
    return clip;
}

AudioClip AudioGenerator::GenerateEngineLoop(float baseFreqHz, float durationSec,
                                              int32_t sampleRate, uint64_t seed) {
    int32_t n = (int32_t)(sampleRate * durationSec);
    AudioClip clip; clip.sampleRate = sampleRate; clip.samples.resize((size_t)n);
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<float> dis(-0.1f, 0.1f);
    const float twoPi = 2.f * 3.14159265359f;
    for (int32_t i = 0; i < n; ++i) {
        float t = (float)i / sampleRate;
        float v = 0.4f * std::sin(twoPi * baseFreqHz * t)
                + 0.3f * std::sin(twoPi * baseFreqHz * 2.f * t)
                + 0.2f * std::sin(twoPi * baseFreqHz * 3.f * t)
                + 0.1f * dis(rng);
        v *= (1.f + 0.1f * std::sin(twoPi * 5.f * t));
        clip.samples[i] = FloatToInt16(v * 0.5f);
    }
    return clip;
}

AudioClip AudioGenerator::GenerateWhiteNoise(float durationSec, float amplitude,
                                              int32_t sampleRate, uint64_t seed) {
    int32_t n = (int32_t)(sampleRate * durationSec);
    AudioClip clip; clip.sampleRate = sampleRate; clip.samples.resize((size_t)n);
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<float> dis(-1.f, 1.f);
    for (int32_t i = 0; i < n; ++i)
        clip.samples[i] = FloatToInt16(amplitude * dis(rng));
    return clip;
}

AudioClip AudioGenerator::GenerateAmbient(float baseFreqHz, float durationSec,
                                           int32_t sampleRate, uint64_t seed) {
    // Layered sine tones with slow LFO modulation
    std::vector<float> freqs  = { baseFreqHz, baseFreqHz*1.5f, baseFreqHz*2.f,
                                  baseFreqHz*3.f };
    std::vector<float> amps   = { 0.4f, 0.25f, 0.2f, 0.15f };
    int32_t n = (int32_t)(sampleRate * durationSec);
    AudioClip clip; clip.sampleRate = sampleRate; clip.samples.resize((size_t)n);
    const float twoPi = 2.f * 3.14159265359f;
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<float> phase(0.f, twoPi);
    std::vector<float> phases(freqs.size());
    for (auto& ph : phases) ph = phase(rng);

    for (int32_t i = 0; i < n; ++i) {
        float t = (float)i / sampleRate;
        float v = 0.f;
        for (size_t j = 0; j < freqs.size(); ++j)
            v += amps[j] * std::sin(twoPi * freqs[j] * t + phases[j]);
        // Slow tremolo
        v *= (0.85f + 0.15f * std::sin(twoPi * 0.2f * t));
        clip.samples[i] = FloatToInt16(v);
    }
    return clip;
}

AudioClip AudioGenerator::Synthesize(const std::vector<OscillatorLayer>& layers,
                                      float durationSec, int32_t sampleRate) {
    int32_t n = (int32_t)(sampleRate * durationSec);
    AudioClip clip; clip.sampleRate = sampleRate; clip.samples.resize((size_t)n);
    const float twoPi = 2.f * 3.14159265359f;
    for (int32_t i = 0; i < n; ++i) {
        float t = (float)i / sampleRate;
        float v = 0.f;
        for (const auto& osc : layers) {
            float wave = 0.f;
            switch (osc.type) {
            case WaveType::Sine:
                wave = std::sin(twoPi * osc.frequency * t + osc.phase); break;
            case WaveType::Square:
                wave = (std::sin(twoPi * osc.frequency * t + osc.phase) >= 0.f) ? 1.f : -1.f; break;
            case WaveType::Sawtooth:
                wave = 2.f * std::fmod(osc.frequency * t + osc.phase / twoPi, 1.f) - 1.f; break;
            case WaveType::Triangle: {
                float ph = std::fmod(osc.frequency * t + osc.phase / twoPi, 1.f);
                wave = (ph < 0.5f) ? (4.f * ph - 1.f) : (3.f - 4.f * ph); break;
            }
            case WaveType::Noise:
                wave = 2.f * ((float)(std::hash<int32_t>{}(i) & 0xFFFF) / 0xFFFF) - 1.f; break;
            }
            v += osc.amplitude * wave;
        }
        if (!layers.empty()) v /= (float)layers.size();
        clip.samples[i] = FloatToInt16(v);
    }
    return clip;
}

void AudioGenerator::ApplyDecayEnvelope(AudioClip& clip, float decayRate) {
    int32_t n = (int32_t)clip.samples.size();
    for (int32_t i = 0; i < n; ++i) {
        float t   = (float)i / n;
        float env = std::exp(-decayRate * t);
        float v   = (float)clip.samples[i] / 32767.f * env;
        clip.samples[i] = FloatToInt16(v);
    }
}

void AudioGenerator::Normalize(AudioClip& clip, float targetPeak) {
    if (clip.samples.empty()) return;
    float maxV = 0.f;
    for (auto s : clip.samples)
        maxV = std::max(maxV, std::abs((float)s));
    if (maxV < 1.f) return;
    float scale = targetPeak * 32767.f / maxV;
    for (auto& s : clip.samples)
        s = (int16_t)std::clamp((float)s * scale, -32767.f, 32767.f);
}

} // namespace PCG
