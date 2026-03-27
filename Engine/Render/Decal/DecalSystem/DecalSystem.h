#pragma once
/**
 * @file DecalSystem.h
 * @brief Projected decals onto world geometry: placement, lifetime, blend modes.
 *
 * Features:
 *   - Decal volume: oriented box projected onto scene geometry
 *   - Per-decal: position, orientation (quat), halfExtents, lifetime, fade-in/out
 *   - Blend modes: Additive, AlphaBlend, Multiply
 *   - Material: albedo colour, alpha, texture slot ID
 *   - Fade-in / fade-out time with current alpha computation
 *   - Layer mask: which surface layers receive decals (32-bit)
 *   - Max decal budget; oldest are culled when budget exceeded
 *   - Tick: advances lifetime, removes expired decals, fires on-expire callback
 *   - Render list: GetVisibleDecals(frustumPlanes) for renderer
 *   - Decal categories for selective clearing
 *
 * Typical usage:
 * @code
 *   DecalSystem ds;
 *   ds.Init(128);
 *   DecalDesc d;
 *   d.position = {x,y,z}; d.halfExtents={0.5f,0.1f,0.5f}; d.lifetime=5.f;
 *   uint32_t id = ds.Spawn(d);
 *   ds.Tick(dt);
 *   auto list = ds.GetVisibleDecals(planes);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class DecalBlend : uint8_t { AlphaBlend=0, Additive, Multiply };

struct DecalDesc {
    float       position  [3]{};
    float       orientation[4]{0,0,0,1};  ///< quaternion
    float       halfExtents[3]{0.5f,0.1f,0.5f};
    float       lifetime  {-1.f};  ///< -1 = infinite
    float       fadeIn    {0.2f};
    float       fadeOut   {0.5f};
    DecalBlend  blend     {DecalBlend::AlphaBlend};
    float       colour    [4]{1,1,1,1};
    uint32_t    textureId {0};
    uint32_t    layerMask {0xFF};
    std::string category;
    bool        receiveGI {false};
};

struct DecalInstance {
    uint32_t  id{0};
    DecalDesc desc;
    float     age{0.f};
    float     alpha{0.f};  ///< computed each tick
    bool      alive{true};
};

class DecalSystem {
public:
    DecalSystem();
    ~DecalSystem();

    void Init    (uint32_t maxDecals);
    void Shutdown();
    void Tick    (float dt);

    // Spawning
    uint32_t Spawn  (const DecalDesc& desc);
    void     Destroy(uint32_t id);
    bool     Has    (uint32_t id) const;

    // Modify live decal
    void SetPosition   (uint32_t id, const float pos[3]);
    void SetOrientation(uint32_t id, const float quat[4]);
    void SetColour     (uint32_t id, const float rgba[4]);
    void SetLifetime   (uint32_t id, float lifetime);

    // Query
    const DecalInstance* Get   (uint32_t id) const;
    uint32_t             Count ()            const;
    float                GetAlpha(uint32_t id) const;

    // Render list
    std::vector<uint32_t> GetVisibleDecals(const float frustumPlanes[24]) const;
    std::vector<uint32_t> GetAll()                                         const;
    std::vector<uint32_t> GetByCategory(const std::string& cat)            const;

    // Clear
    void ClearAll      ();
    void ClearCategory (const std::string& cat);

    // Callbacks
    void SetOnExpire(std::function<void(uint32_t id)> cb);

    // Budget
    void     SetMaxDecals(uint32_t n);
    uint32_t MaxDecals()  const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
