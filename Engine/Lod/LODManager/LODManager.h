#pragma once
/**
 * @file LODManager.h
 * @brief Level-of-Detail manager for meshes, textures, and actors.
 *
 * Each registered object has a set of LOD levels (mesh or detail variants)
 * selected based on the camera's distance and a configurable bias.
 * Also handles HLOD (Hierarchical LOD) cluster merging for very distant groups.
 *
 * Features:
 *   - Per-object LOD thresholds (world-space distance)
 *   - Global LOD bias (quality setting)
 *   - Texture streaming LOD (mip-level selection)
 *   - Screen-size based selection (uses projected size in pixels)
 *   - LOD transition cross-fade (dithered alpha blend)
 *
 * Typical usage:
 * @code
 *   LODManager lm;
 *   lm.Init();
 *   uint32_t id = lm.RegisterObject("rock_01", {
 *       {0.f, "Meshes/rock_01_lod0.mesh"},
 *       {20.f,"Meshes/rock_01_lod1.mesh"},
 *       {60.f,"Meshes/rock_01_lod2.mesh"},
 *   });
 *   lm.SetCameraPosition({0,0,0});
 *   lm.Update(dt);
 *   int level = lm.GetCurrentLOD(id);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct LODLevel {
    float       startDistance{0.f};   ///< use this level beyond this distance
    std::string meshOrAssetPath;
    float       screenSizeThreshold{0.f}; ///< alternative: projected size %
};

struct LODObject {
    uint32_t              id{0};
    std::string           name;
    float                 position[3]{};
    std::vector<LODLevel> levels;
    int                   currentLevel{0};
    float                 crossFadeAlpha{1.f};
    bool                  active{true};
};

struct LODSettings {
    float globalBias{1.f};        ///< >1 = favour higher detail
    bool  screenSizeMode{false};  ///< use projected size instead of distance
    float crossFadeTime{0.3f};
    bool  hlodEnabled{false};
};

class LODManager {
public:
    LODManager();
    ~LODManager();

    void Init(const LODSettings& settings = {});
    void Shutdown();

    uint32_t RegisterObject(const std::string& name,
                            const std::vector<LODLevel>& levels,
                            const float position[3] = nullptr);
    void     UnregisterObject(uint32_t id);
    void     SetObjectPosition(uint32_t id, const float pos[3]);
    void     SetObjectActive(uint32_t id, bool active);
    LODObject GetObject(uint32_t id) const;

    int      GetCurrentLOD(uint32_t id) const;
    float    GetCrossFadeAlpha(uint32_t id) const;
    std::vector<LODObject> AllObjects() const;

    void SetCameraPosition(const float pos[3]);
    void SetCameraFOV(float fovDeg);
    void SetGlobalBias(float bias);
    void SetSettings(const LODSettings& s);

    void Update(float dt);

    void OnLODChanged(std::function<void(uint32_t id, int oldLevel, int newLevel)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
