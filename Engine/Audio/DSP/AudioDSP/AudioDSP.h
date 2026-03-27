#pragma once
/**
 * @file AudioDSP.h
 * @brief CPU DSP effect chain: EQ, reverb, chorus, delay, compressor applied to float buffers.
 *
 * Features:
 *   - CreateChain(id): create a named effect chain
 *   - AddEffect(chainId, effectType, params): append DSP effect to chain
 *   - RemoveEffect(chainId, effectIndex)
 *   - Process(chainId, samples, count, sampleRate): apply full chain in-place
 *   - Effect types: LowPass, HighPass, BandPass, Reverb, Chorus, Delay, Compressor, Gain
 *   - Per-effect parameter struct (cutoff, resonance, decay, mix, threshold, ratio…)
 *   - SetEffectParam(chainId, effectIndex, paramName, value)
 *   - SetEffectEnabled(chainId, effectIndex, enabled)
 *   - MoveEffect(chainId, fromIndex, toIndex): reorder in chain
 *   - GetChainLatency(chainId) → uint32_t samples
 *   - Reset(chainId): flush all internal effect state (delay lines, buffers)
 *   - Shutdown() / Init()
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Engine {

enum class DSPEffectType : uint8_t {
    LowPass, HighPass, BandPass,
    Reverb, Chorus, Delay,
    Compressor, Gain
};

struct DSPParams {
    float cutoff    {1000.f}; ///< Hz, for filters
    float resonance {0.707f}; ///< Q factor
    float decay     {0.5f};   ///< seconds (reverb/delay)
    float mix       {0.5f};   ///< wet/dry [0,1]
    float threshold {-20.f};  ///< dB (compressor)
    float ratio     {4.f};    ///< compression ratio
    float gain      {1.f};    ///< linear gain (Gain effect)
    float rate      {0.5f};   ///< Hz (chorus LFO)
    float depth     {0.002f}; ///< seconds (chorus depth)
};

class AudioDSP {
public:
    AudioDSP();
    ~AudioDSP();

    void Init    ();
    void Shutdown();

    // Chain management
    uint32_t CreateChain  (const std::string& name="");
    void     DestroyChain (uint32_t chainId);

    // Effect authoring
    uint32_t AddEffect    (uint32_t chainId, DSPEffectType type, const DSPParams& params={});
    void     RemoveEffect (uint32_t chainId, uint32_t effectIndex);
    void     MoveEffect   (uint32_t chainId, uint32_t fromIndex, uint32_t toIndex);

    // Processing
    void Process(uint32_t chainId, float* samples, uint32_t count, uint32_t sampleRate=44100);

    // Per-effect control
    void SetEffectParam  (uint32_t chainId, uint32_t effectIndex,
                          const std::string& param, float value);
    void SetEffectEnabled(uint32_t chainId, uint32_t effectIndex, bool enabled);

    // Query
    uint32_t GetEffectCount  (uint32_t chainId) const;
    uint32_t GetChainLatency (uint32_t chainId) const; ///< samples of algorithmic delay
    void     Reset           (uint32_t chainId);       ///< flush buffers
    DSPParams GetEffectParams(uint32_t chainId, uint32_t effectIndex) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
