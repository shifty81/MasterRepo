#pragma once
/**
 * @file AudioMixer.h
 * @brief Multi-bus mixing: per-bus volume/pitch/pan, send/return chains, limiter.
 *
 * Features:
 *   - Bus hierarchy: Master bus + named sub-buses (Music, SFX, Voice, Ambience…)
 *   - Per-bus: volume (0-1+), pitch multiplier, stereo pan (-1..1), mute, solo
 *   - Bus routing: child buses feed into parent bus
 *   - Send/Return: configurable wet-mix sends between buses
 *   - Per-bus limiter/compressor: threshold, ratio, attack, release
 *   - Per-bus low-pass filter: cutoff frequency
 *   - VU meter: peak and RMS per bus
 *   - Thread-safe: SetVolume/SetPitch safe from audio thread
 *   - Snapshot system: save/restore named mixer states with blend time
 *   - Tick: advance compressor/limiter envelopes
 *
 * Typical usage:
 * @code
 *   AudioMixer am;
 *   am.Init();
 *   uint32_t sfx = am.AddBus("SFX", am.MasterBus());
 *   am.SetVolume(sfx, 0.8f);
 *   am.SetLimiter(sfx, {-6.f, 4.f, 5.f, 50.f});
 *   am.Tick(dt);
 *   auto vu = am.GetVU(sfx);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct LimiterDesc {
    float thresholdDB {-6.f};
    float ratio       {4.f};
    float attackMs    {5.f};
    float releaseMs   {50.f};
    bool  enabled     {true};
};

struct LPFDesc {
    float cutoffHz{20000.f};
    bool  enabled{false};
};

struct VUMeter {
    float peakDB {-96.f};
    float rmsDB  {-96.f};
};

struct MixerSnapshot {
    std::string name;
    struct BusState {
        uint32_t busId{0};
        float    volume{1.f};
        float    pitch {1.f};
        float    pan   {0.f};
        bool     muted {false};
    };
    std::vector<BusState> buses;
};

class AudioMixer {
public:
    AudioMixer();
    ~AudioMixer();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    // Bus management
    uint32_t    AddBus   (const std::string& name, uint32_t parentBus=0);
    void        RemoveBus(uint32_t busId);
    uint32_t    MasterBus() const;
    uint32_t    FindBus  (const std::string& name) const;
    bool        HasBus   (uint32_t busId) const;
    std::string BusName  (uint32_t busId) const;

    // Per-bus controls
    void  SetVolume(uint32_t busId, float v);        ///< 0-1
    void  SetPitch (uint32_t busId, float p);        ///< 1=normal
    void  SetPan   (uint32_t busId, float pan);      ///< -1..1
    void  SetMute  (uint32_t busId, bool muted);
    void  SetSolo  (uint32_t busId, bool solo);
    float GetVolume(uint32_t busId) const;
    float GetPitch (uint32_t busId) const;
    float GetPan   (uint32_t busId) const;
    bool  IsMuted  (uint32_t busId) const;
    bool  IsSoloed (uint32_t busId) const;

    // Computed effective volume (including parent chain, mute/solo)
    float GetEffectiveVolume(uint32_t busId) const;

    // FX
    void SetLimiter(uint32_t busId, const LimiterDesc& desc);
    void SetLPF    (uint32_t busId, const LPFDesc&     desc);

    // Send / return (wet signal send from src to dst)
    void  AddSend   (uint32_t srcBus, uint32_t dstBus, float sendLevel=0.f);
    void  SetSend   (uint32_t srcBus, uint32_t dstBus, float sendLevel);
    void  RemoveSend(uint32_t srcBus, uint32_t dstBus);

    // VU
    VUMeter GetVU(uint32_t busId) const;

    // Snapshots
    void    SaveSnapshot  (const std::string& name);
    bool    ApplySnapshot (const std::string& name, float blendTime=0.f);
    std::vector<std::string> GetSnapshots() const;

    // Enumerate
    std::vector<uint32_t> GetAllBuses() const;
    uint32_t              BusCount()    const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
