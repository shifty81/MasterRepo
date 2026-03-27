#pragma once
/**
 * @file AudioAnalyzer.h
 * @brief FFT spectrum, beat detection, RMS/peak metering, frequency-band splitter.
 *
 * Features:
 *   - Submit(samples, count, sampleRate): feed PCM float frames
 *   - GetSpectrum(outMagnitudes, numBins): FFT magnitude array (Hann-windowed)
 *   - GetRMS() / GetPeak() → float [0,1]
 *   - BeatDetect: energy-ratio method, OnBeat callback, BPM estimator
 *   - FrequencyBand splitter: sub-bass, bass, mid, high, presence, brilliance
 *   - GetBandEnergy(BandId) → float
 *   - SpectrumHistory: rolling N frames for waterfall display
 *   - SetFFTSize(512/1024/2048/4096)
 *   - Reset()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

enum class FreqBand : uint8_t { SubBass=0, Bass, Mid, High, Presence, Brilliance, _Count };

class AudioAnalyzer {
public:
    AudioAnalyzer();
    ~AudioAnalyzer();

    void Init    (uint32_t fftSize=1024);
    void Shutdown();
    void Reset   ();

    // Feed raw PCM (interleaved mono or single channel)
    void Submit(const float* samples, uint32_t count, uint32_t sampleRate=44100);

    // Spectrum
    void     GetSpectrum  (float* outMagnitudes, uint32_t numBins) const;
    uint32_t FFTSize      () const;
    uint32_t BinCount     () const; ///< FFTSize/2

    // Metering
    float GetRMS () const;
    float GetPeak() const;

    // Frequency bands
    float GetBandEnergy(FreqBand band) const;

    // Beat detection
    bool  IsBeat       () const;     ///< true on the frame a beat is detected
    float GetEstBPM    () const;
    void  SetOnBeat    (std::function<void(float bpm)> cb);
    void  SetBeatThreshold(float t); ///< energy-ratio threshold, default 1.3

    // Spectrum history (last N frames, each BinCount floats)
    const std::vector<std::vector<float>>& GetHistory() const;
    void SetHistoryLength(uint32_t frames);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
