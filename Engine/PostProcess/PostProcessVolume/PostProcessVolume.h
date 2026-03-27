#pragma once
/**
 * @file PostProcessVolume.h
 * @brief In-world volumes that blend post-process settings by actor proximity.
 *
 * Features:
 *   - Define PP volumes as AABB or sphere with blend radius
 *   - Per-volume settings: bloom intensity/threshold, exposure/AO strength,
 *     chromatic aberration, vignette, colour grading LUT, depth-of-field, fog
 *   - Priority system: higher-priority volumes override lower when overlapping
 *   - Soft blend: settings lerp over blend radius entering/exiting volume
 *   - Global (unbound) volume as the world baseline
 *   - Track query actor(s): register actor positions each frame, get blended result
 *   - On-enter / on-exit callbacks per volume
 *   - JSON serialisation
 *
 * Typical usage:
 * @code
 *   PostProcessVolume ppv;
 *   ppv.Init();
 *   // World baseline
 *   PostProcessSettings base;
 *   base.bloomIntensity = 0.2f;
 *   ppv.SetGlobal(base);
 *   // Cave volume
 *   PostProcessSettings cave; cave.fogDensity=0.3f; cave.exposure=-1.f;
 *   ppv.AddVolume({"cave_1", VolumeShape::AABB, {10,0,10}, {5,3,5}, 1.f, cave});
 *   // per-frame
 *   ppv.UpdateActorPosition(cameraId, cameraPos);
 *   auto pp = ppv.GetBlendedSettings(cameraId);
 *   Renderer::ApplyPostProcess(pp);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct PostProcessSettings {
    // Bloom
    float bloomIntensity{0.3f};
    float bloomThreshold{0.8f};
    // Exposure / tone mapping
    float exposure{0.f};           ///< EV offset
    float saturation{1.f};
    float contrast{1.f};
    // Ambient occlusion
    float aoStrength{0.5f};
    float aoRadius{0.3f};
    // Chromatic aberration
    float chromaticAberration{0.f};
    // Vignette
    float vignetteIntensity{0.0f};
    float vignetteRadius{0.7f};
    // Depth of field
    bool  dofEnabled{false};
    float dofFocalDistance{10.f};
    float dofFocalRange{5.f};
    float dofBlurRadius{3.f};
    // Fog
    float fogDensity{0.f};
    float fogColour[3]{0.6f,0.7f,0.8f};
    // LUT
    std::string colourGradingLUT;
};

// Lerp two PP settings
PostProcessSettings LerpPP(const PostProcessSettings& a,
                            const PostProcessSettings& b, float t);

enum class VolumeShape : uint8_t { AABB=0, Sphere };

struct PPVolumeDesc {
    std::string          id;
    VolumeShape          shape{VolumeShape::AABB};
    float                centre[3]{};
    float                halfExtents[3]{1,1,1}; ///< for AABB; [0]=radius for Sphere
    float                blendRadius{1.f};       ///< outer blend zone
    int32_t              priority{0};
    PostProcessSettings  settings;
    bool                 exclusive{false};       ///< if true, ignore lower-priority volumes
};

class PostProcessVolume {
public:
    PostProcessVolume();
    ~PostProcessVolume();

    void Init();
    void Shutdown();

    // Volume management
    void  AddVolume(const PPVolumeDesc& desc);
    void  RemoveVolume(const std::string& id);
    void  UpdateVolume(const std::string& id, const PPVolumeDesc& desc);
    bool  HasVolume(const std::string& id) const;

    // Global baseline
    void SetGlobal(const PostProcessSettings& settings);
    const PostProcessSettings& GetGlobal() const;

    // Per-actor tracking
    void UpdateActorPosition(uint32_t actorId, const float pos[3]);
    void RemoveActor(uint32_t actorId);

    // Query (returns blended settings for actor)
    PostProcessSettings GetBlendedSettings(uint32_t actorId) const;
    PostProcessSettings GetBlendedSettingsAt(const float pos[3]) const;

    // Callbacks
    using VolumeCb = std::function<void(uint32_t actorId, const std::string& volumeId)>;
    void SetOnVolumeEnter(VolumeCb cb);
    void SetOnVolumeExit(VolumeCb cb);

    // Serialisation
    bool SaveJSON(const std::string& path) const;
    bool LoadJSON(const std::string& path);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
