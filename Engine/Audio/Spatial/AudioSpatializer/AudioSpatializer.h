#pragma once
/**
 * @file AudioSpatializer.h
 * @brief 3D positional audio: HRTF-inspired panning, distance attenuation, Doppler, reverb send.
 *
 * Features:
 *   - RegisterSource(id, pos, vel): add a spatial audio emitter
 *   - RemoveSource(id)
 *   - SetSourcePos(id, pos) / SetSourceVel(id, vel)
 *   - SetListenerPos(pos, vel, forward, up): update listener state
 *   - SetAttenuationModel(model): None/Linear/InverseSquare/LogarithmicTable
 *   - SetRolloffFactor(f): scale attenuation rate
 *   - SetRefDistance(d) / SetMaxDistance(d): attenuation envelope distances
 *   - EnableDoppler(on) / SetDopplerFactor(f)
 *   - EnableReverb(on) / SetReverbSend(id, send): per-source reverb mix [0,1]
 *   - Evaluate(sourceId, dt) → SpatialOutput: {leftGain, rightGain, pitch, reverbSend}
 *   - EvaluateAll(dt, outArray[]): batch evaluation for all sources
 *   - SetOnCull(id, cb): callback when source moves beyond maxDistance
 *   - GetSourceCount() → uint32_t
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

enum class AttenuationModel : uint8_t { None, Linear, InverseSquare, Logarithmic };

struct ASVec3 { float x,y,z; };

struct SpatialOutput {
    float leftGain  {1};
    float rightGain {1};
    float pitchShift{1}; ///< Doppler factor
    float reverbSend{0};
};

class AudioSpatializer {
public:
    AudioSpatializer();
    ~AudioSpatializer();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Listener
    void SetListenerPos(ASVec3 pos, ASVec3 vel, ASVec3 forward, ASVec3 up);

    // Source management
    void RegisterSource(uint32_t id, ASVec3 pos, ASVec3 vel={0,0,0});
    void RemoveSource  (uint32_t id);
    void SetSourcePos  (uint32_t id, ASVec3 pos);
    void SetSourceVel  (uint32_t id, ASVec3 vel);

    // Configuration
    void SetAttenuationModel(AttenuationModel model);
    void SetRolloffFactor   (float f);
    void SetRefDistance     (float d);
    void SetMaxDistance     (float d);
    void EnableDoppler      (bool on);
    void SetDopplerFactor   (float f);
    void EnableReverb       (bool on);
    void SetReverbSend      (uint32_t sourceId, float send);

    // Evaluation
    SpatialOutput              Evaluate   (uint32_t sourceId, float dt) const;
    std::vector<SpatialOutput> EvaluateAll(float dt) const;

    // Callbacks
    void SetOnCull(uint32_t sourceId, std::function<void()> cb);

    uint32_t GetSourceCount() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
