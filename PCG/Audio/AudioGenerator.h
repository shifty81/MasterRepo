#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace PCG {

// ──────────────────────────────────────────────────────────────
// Procedural audio types
// ──────────────────────────────────────────────────────────────

enum class WaveType { Sine, Square, Sawtooth, Triangle, Noise };

struct OscillatorLayer {
    WaveType type      = WaveType::Sine;
    float    frequency = 440.f;   // Hz
    float    amplitude = 1.0f;    // 0–1
    float    phase     = 0.0f;    // radians
};

struct AudioClip {
    int32_t              sampleRate = 44100;
    std::vector<int16_t> samples;

    float DurationSeconds() const {
        return sampleRate > 0 ? (float)samples.size() / (float)sampleRate : 0.f;
    }
    bool SaveWAV(const std::string& path) const;
};

// ──────────────────────────────────────────────────────────────
// AudioGenerator — PCG namespace, migrated from EVE::AudioGenerator
// ──────────────────────────────────────────────────────────────

class AudioGenerator {
public:
    // Primitive generators
    static AudioClip GenerateTone(float frequencyHz,
                                  float durationSec,
                                  int32_t sampleRate = 44100,
                                  float amplitude    = 0.8f);

    static AudioClip GenerateMultiTone(const std::vector<float>& frequencies,
                                       const std::vector<float>& amplitudes,
                                       float durationSec,
                                       int32_t sampleRate = 44100);

    // Preset sound effects (from EVE::AudioGenerator)
    static AudioClip GenerateExplosion(float durationSec = 1.5f,
                                       int32_t sampleRate = 44100,
                                       uint64_t seed      = 0);

    static AudioClip GenerateLaser(float startFreqHz  = 2000.f,
                                   float endFreqHz    = 500.f,
                                   float durationSec  = 0.3f,
                                   int32_t sampleRate = 44100);

    static AudioClip GenerateEngineLoop(float baseFreqHz  = 120.f,
                                        float durationSec  = 2.0f,
                                        int32_t sampleRate = 44100,
                                        uint64_t seed      = 0);

    // Ambient generators
    static AudioClip GenerateWhiteNoise(float durationSec,
                                        float amplitude    = 0.5f,
                                        int32_t sampleRate = 44100,
                                        uint64_t seed      = 0);

    static AudioClip GenerateAmbient(float baseFreqHz  = 60.f,
                                     float durationSec  = 4.0f,
                                     int32_t sampleRate = 44100,
                                     uint64_t seed      = 0);

    // Multi-layer oscillator synthesis
    static AudioClip Synthesize(const std::vector<OscillatorLayer>& layers,
                                float durationSec,
                                int32_t sampleRate = 44100);

    // Apply exponential decay envelope in-place
    static void ApplyDecayEnvelope(AudioClip& clip, float decayRate = 3.0f);

    // Normalise peak amplitude to targetPeak (0–1)
    static void Normalize(AudioClip& clip, float targetPeak = 0.9f);

    static int16_t FloatToInt16(float v);
    static bool    WriteWAVHeader(std::ostream& out,
                                  int32_t sampleRate,
                                  int32_t numSamples,
                                  int32_t bitsPerSample = 16,
                                  int32_t numChannels   = 1);
};

} // namespace PCG
