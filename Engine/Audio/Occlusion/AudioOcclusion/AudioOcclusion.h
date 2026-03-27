#pragma once
/**
 * @file AudioOcclusion.h
 * @brief Ray-based audio obstruction/occlusion: per-source material filter, reverb send.
 *
 * Features:
 *   - OcclusionSource: world position, audio source id, max range
 *   - Pluggable ray-cast function: OcclusionRayFn(from, to) → OcclusionHit{material,distance}
 *   - Material table: material id → lpf cutoff scale, obstruction db loss
 *   - Direct-path occlusion: fire ray listener→source, accumulate material losses
 *   - Reverb send level: derived from hit count / total path length
 *   - Per-source result: occlusionFactor [0,1], lpfCutoff (Hz), reverbSend [0,1]
 *   - Tick: update all active sources each frame
 *   - GetResult(sourceId) → OcclusionResult
 *   - Smooth lerp of results over time (configurable attack/release)
 *   - Multiple listener positions (split-screen support)
 *
 * Typical usage:
 * @code
 *   AudioOcclusion ao;
 *   ao.Init();
 *   ao.SetRayFn([](const float a[3], const float b[3]) {
 *       return RaycastWorld(a, b);
 *   });
 *   ao.RegisterMaterial(1, {0.4f, -12.f}); // concrete
 *   uint32_t sid = ao.AddSource({worldPos, audioId, 30.f});
 *   ao.SetListenerPos({0,1.7f,0});
 *   ao.Tick(dt);
 *   auto r = ao.GetResult(sid); // r.occlusionFactor, r.lpfCutoff
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct OcclusionMaterial {
    float lpfCutoffScale {0.5f};  ///< multiply base cutoff (e.g. 0.3 = very muffled)
    float dbLoss         {-6.f};  ///< dB attenuation per hit
};

struct OcclusionHit {
    uint8_t materialId{0};
    float   distance  {0.f};
    bool    hit       {false};
};

struct OcclusionSourceDesc {
    float    worldPos[3]{};
    uint32_t audioSourceId{0};
    float    maxRange{50.f};
};

struct OcclusionResult {
    float occlusionFactor{0.f};   ///< 0=open, 1=fully occluded
    float lpfCutoff      {20000.f};
    float reverbSend     {0.f};   ///< 0-1
};

using OcclusionRayFn = std::function<OcclusionHit(const float from[3], const float to[3])>;

class AudioOcclusion {
public:
    AudioOcclusion();
    ~AudioOcclusion();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    void SetRayFn      (OcclusionRayFn fn);
    void SetListenerPos(const float pos[3], uint32_t listenerId=0);
    void SetBaseLpf    (float hz);         ///< default 20000 Hz
    void SetSmoothTime (float attackSec, float releaseSec);

    // Materials
    void RegisterMaterial(uint8_t materialId, const OcclusionMaterial& mat);

    // Sources
    uint32_t AddSource   (const OcclusionSourceDesc& desc);
    void     RemoveSource(uint32_t id);
    void     UpdatePos   (uint32_t id, const float newPos[3]);
    bool     HasSource   (uint32_t id) const;

    // Results
    OcclusionResult GetResult(uint32_t id) const;

    std::vector<uint32_t> GetAll() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
