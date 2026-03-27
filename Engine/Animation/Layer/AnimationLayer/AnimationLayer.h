#pragma once
/**
 * @file AnimationLayer.h
 * @brief Additive/override animation layering with mask, weight, blend-in/out.
 *
 * Features:
 *   - LayerMode: Override, Additive
 *   - AddLayer(layerId, mode, weight, boneMask[]) → bool
 *   - RemoveLayer(layerId)
 *   - SetLayerWeight(layerId, w): blend weight [0,1]
 *   - GetLayerWeight(layerId) → float
 *   - SetLayerMode(layerId, mode)
 *   - SetBoneMask(layerId, boneIds[]): only these bones affected
 *   - PlayClip(layerId, clipName, loop) → bool
 *   - StopClip(layerId)
 *   - BlendIn (layerId, duration): fade weight 0→target over duration
 *   - BlendOut(layerId, duration): fade weight target→0 then stop
 *   - Tick(dt): advance clip times, apply blend-in/out
 *   - GetCurrentClip(layerId) → string
 *   - GetClipTime(layerId) → float
 *   - IsPlaying(layerId) → bool
 *   - GetLayerCount() → uint32_t
 *   - SetOnComplete(layerId, cb): called when clip ends (non-looping)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class LayerMode : uint8_t { Override, Additive };

class AnimationLayer {
public:
    AnimationLayer();
    ~AnimationLayer();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Layer management
    bool AddLayer   (uint32_t layerId, LayerMode mode, float weight,
                     const std::vector<uint32_t>& boneMask = {});
    void RemoveLayer(uint32_t layerId);

    // Layer config
    void      SetLayerWeight(uint32_t layerId, float w);
    float     GetLayerWeight(uint32_t layerId) const;
    void      SetLayerMode  (uint32_t layerId, LayerMode mode);
    void      SetBoneMask   (uint32_t layerId,
                              const std::vector<uint32_t>& boneIds);

    // Clip playback
    bool PlayClip(uint32_t layerId, const std::string& clip, bool loop = false);
    void StopClip(uint32_t layerId);

    // Blending
    void BlendIn (uint32_t layerId, float duration);
    void BlendOut(uint32_t layerId, float duration);

    // Per-frame
    void Tick(float dt);

    // Query
    std::string GetCurrentClip  (uint32_t layerId) const;
    float       GetClipTime     (uint32_t layerId) const;
    bool        IsPlaying       (uint32_t layerId) const;
    uint32_t    GetLayerCount   () const;

    // Callback
    void SetOnComplete(uint32_t layerId,
                       std::function<void()> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
