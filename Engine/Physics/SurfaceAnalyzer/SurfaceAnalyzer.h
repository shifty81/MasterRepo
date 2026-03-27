#pragma once
/**
 * @file SurfaceAnalyzer.h
 * @brief Surface-type detection for physics materials and audio/VFX dispatch.
 *
 * Features:
 *   - Tag-based surface material registry (concrete, dirt, metal, water, …)
 *   - Raycast / contact-point → surface tag resolution
 *   - Per-surface physics properties: friction, restitution, roll resistance
 *   - Per-surface audio/VFX hints: step sound, impact sound, slide sound
 *   - Blend weight support for multi-surface terrain (e.g. 60% grass + 40% sand)
 *   - Async batch-query API for many probes per frame
 *   - Integration hooks: supply a raycast function and an entity-tag lookup
 *
 * Typical usage:
 * @code
 *   SurfaceAnalyzer sa;
 *   sa.Init();
 *   sa.RegisterSurface({"concrete", 0.7f, 0.3f, "sfx_step_concrete"});
 *   sa.RegisterSurface({"water",    0.1f, 0.0f, "sfx_step_splash"});
 *   sa.SetRaycastFn([](const Ray& r, SurfaceHit& h){ return MyPhysics::Raycast(r,h); });
 *   SurfaceResult res = sa.Analyze({playerFoot}, {0,-1,0}, 0.5f);
 *   ApplyFriction(res.friction);
 *   PlayStepSound(res.stepSound);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct SurfaceMaterial {
    std::string  tag;                   ///< unique identifier e.g. "concrete"
    float        friction{0.6f};        ///< Coulomb friction coefficient
    float        restitution{0.2f};     ///< bounciness [0,1]
    float        rollResistance{0.01f};
    float        snowDepth{0.f};        ///< visual/gameplay depth modifier
    std::string  stepSound;
    std::string  impactSound;
    std::string  slideSound;
    std::string  stepParticle;
    std::string  impactParticle;
};

struct Ray {
    float origin[3]{};
    float direction[3]{0,-1,0};
};

struct SurfaceHit {
    float        point[3]{};
    float        normal[3]{0,1,0};
    float        distance{0.f};
    uint32_t     entityId{0};     ///< entity that was hit (0 = world geometry)
    std::string  materialTag;     ///< filled by SurfaceAnalyzer after lookup
};

struct SurfaceBlendWeight {
    std::string  tag;
    float        weight{1.f};
};

struct SurfaceResult {
    std::string  primaryTag;
    float        friction{0.6f};
    float        restitution{0.2f};
    std::string  stepSound;
    std::string  impactSound;
    std::string  stepParticle;
    bool         hit{false};
    float        hitPoint[3]{};
    float        hitNormal[3]{0,1,0};
    std::vector<SurfaceBlendWeight> blends; ///< multi-surface contribution
};

using RaycastFn     = std::function<bool(const Ray&, SurfaceHit&)>;
using EntityTagFn   = std::function<std::string(uint32_t entityId)>;

class SurfaceAnalyzer {
public:
    SurfaceAnalyzer();
    ~SurfaceAnalyzer();

    void Init();
    void Shutdown();

    // Surface registry
    void   RegisterSurface(const SurfaceMaterial& mat);
    void   UnregisterSurface(const std::string& tag);
    bool   HasSurface(const std::string& tag) const;
    const  SurfaceMaterial* GetSurface(const std::string& tag) const;

    // Integration
    void   SetRaycastFn(RaycastFn fn);
    void   SetEntityTagFn(EntityTagFn fn);

    // Analysis
    SurfaceResult Analyze(const float origin[3], const float dir[3],
                          float maxDist=0.5f) const;
    SurfaceResult AnalyzeContact(const float point[3], uint32_t entityId) const;

    // Batch (returns one result per probe)
    struct Probe { float origin[3]; float dir[3]; float maxDist{0.5f}; };
    std::vector<SurfaceResult> BatchAnalyze(const std::vector<Probe>& probes) const;

    // Blend helper (terrain painted weights)
    SurfaceResult BlendResults(const std::vector<SurfaceBlendWeight>& weights) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
