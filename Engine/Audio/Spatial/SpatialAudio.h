#pragma once
/**
 * @file SpatialAudio.h
 * @brief 3D positional audio with HRTF panning, distance attenuation,
 *        reverb zones, and per-source occlusion.
 *
 * Builds on top of the existing Audio subsystem by adding spatial processing.
 * Each sound source has a world-space position; the listener (camera) position
 * is updated each frame to drive panning and attenuation.
 *
 * Typical usage:
 * @code
 *   SpatialAudio spatial;
 *   SpatialAudioConfig cfg;
 *   cfg.speedOfSound = 343.f;
 *   cfg.dopplerFactor = 1.f;
 *   spatial.Init(cfg);
 *   spatial.SetListenerTransform({0,0,0}, {0,0,-1}, {0,1,0});
 *   uint32_t srcId = spatial.CreateSource("Audio/explosion.wav");
 *   spatial.SetSourcePosition(srcId, {10, 0, 5});
 *   spatial.PlaySource(srcId);
 *   spatial.Update(deltaSeconds);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

// ── Spatial audio config ──────────────────────────────────────────────────────

struct SpatialAudioConfig {
    float speedOfSound{343.f};       ///< m/s for Doppler
    float dopplerFactor{1.f};        ///< 0 = disabled, 1 = realistic
    float rolloffFactor{1.f};
    float referenceDistance{1.f};    ///< distance at which volume = 1
    float maxDistance{200.f};
    bool  hrtfEnabled{true};         ///< head-related transfer function panning
};

// ── Reverb zone ───────────────────────────────────────────────────────────────

struct ReverbZone {
    uint32_t id{0};
    float    center[3]{};
    float    radius{10.f};
    float    decay{0.5f};      ///< reverb tail length in seconds
    float    wetMix{0.3f};     ///< 0–1
    float    predelay{0.01f};  ///< seconds
};

// ── Audio source state ────────────────────────────────────────────────────────

struct SpatialSourceInfo {
    uint32_t    id{0};
    float       position[3]{};
    float       velocity[3]{};  ///< for Doppler
    float       volume{1.f};
    float       pitch{1.f};
    float       minDistance{1.f};
    float       maxDistance{200.f};
    bool        looping{false};
    bool        playing{false};
    bool        occluded{false};
    std::string assetPath;
};

// ── SpatialAudio ──────────────────────────────────────────────────────────────

class SpatialAudio {
public:
    SpatialAudio();
    ~SpatialAudio();

    void Init(const SpatialAudioConfig& config = {});
    void Shutdown();

    // ── Listener ──────────────────────────────────────────────────────────────

    void SetListenerTransform(const float position[3],
                              const float forward[3],
                              const float up[3]);
    void SetListenerVelocity(const float velocity[3]);

    // ── Sound sources ─────────────────────────────────────────────────────────

    uint32_t    CreateSource(const std::string& assetPath);
    void        DestroySource(uint32_t sourceId);

    void        SetSourcePosition(uint32_t id, const float position[3]);
    void        SetSourceVelocity(uint32_t id, const float velocity[3]);
    void        SetSourceVolume(uint32_t id, float volume);
    void        SetSourcePitch(uint32_t id, float pitch);
    void        SetSourceLooping(uint32_t id, bool loop);
    void        SetSourceDistances(uint32_t id, float minDist, float maxDist);

    void        PlaySource(uint32_t id);
    void        StopSource(uint32_t id);
    void        PauseSource(uint32_t id);

    bool        IsPlaying(uint32_t id) const;
    SpatialSourceInfo GetSourceInfo(uint32_t id) const;
    std::vector<uint32_t> ActiveSourceIds() const;

    // ── Reverb zones ──────────────────────────────────────────────────────────

    uint32_t    AddReverbZone(const ReverbZone& zone);
    void        RemoveReverbZone(uint32_t zoneId);
    void        UpdateReverbZone(uint32_t zoneId, const ReverbZone& zone);

    // ── Occlusion ─────────────────────────────────────────────────────────────

    /// Override computed occlusion for a source (0 = fully occluded, 1 = open).
    void SetSourceOcclusion(uint32_t id, float factor);

    // ── Per-frame update ──────────────────────────────────────────────────────

    void Update(float deltaSeconds);

    // ── Global controls ───────────────────────────────────────────────────────

    void SetMasterVolume(float volume);
    void PauseAll();
    void ResumeAll();
    void StopAll();

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnSourceFinished(std::function<void(uint32_t sourceId)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
