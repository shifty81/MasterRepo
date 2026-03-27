#pragma once
/**
 * @file GrassSystem.h
 * @brief GPU-instanced procedural grass: density map, wind, LOD culling, interaction.
 *
 * Features:
 *   - SetTerrainSize(width, height, tileSize): define world-space grass domain
 *   - SetDensityMap(data, w, h): per-texel density [0,1]; 0 = no grass
 *   - SetBladeParams(height, width, bend, taper): grass blade geometry config
 *   - GenerateInstances(): (re-)populate instance positions from density map
 *   - SetWindDirection(dir) / SetWindSpeed(s) / SetWindTurbulence(t)
 *   - SetLODDistances(near, mid, far): per-LOD cull distances
 *   - Cull(cameraPos, forwardDir, fovY, aspect): compute visible instance list
 *   - GetVisibleInstanceCount() → uint32_t
 *   - GetInstanceData(outPos[], outBend[], count): write culled instance transforms
 *   - SetInteraction(pos, radius, strength): flatten/push grass near a point
 *   - ClearInteractions()
 *   - SetSeason(t): 0=summer(green)…1=autumn(yellow) affects tint output
 *   - GetTintColour(t) → RGB: interpolated seasonal tint
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <vector>

namespace Engine {

struct GVec3 { float x,y,z; };
struct GRGB  { float r,g,b; };

struct GrassInstance {
    GVec3 pos;
    float bendAngle;  ///< wind + interaction combined bend
    float scale;
};

struct GrassInteraction {
    GVec3 pos;
    float radius;
    float strength;
};

class GrassSystem {
public:
    GrassSystem();
    ~GrassSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Terrain domain
    void SetTerrainSize(float width, float height, float tileSize);

    // Density
    void SetDensityMap  (const float* data, uint32_t w, uint32_t h);
    void GenerateInstances();

    // Blade config
    void SetBladeParams(float height, float width, float bend, float taper);

    // Wind
    void SetWindDirection (GVec3 dir);
    void SetWindSpeed     (float s);
    void SetWindTurbulence(float t);

    // LOD
    void SetLODDistances(float near, float mid, float far);

    // Culling
    void     Cull                (GVec3 cameraPos, GVec3 forwardDir, float fovY, float aspect);
    uint32_t GetVisibleInstanceCount() const;
    void     GetInstanceData(std::vector<GrassInstance>& out) const;

    // Interactions
    void AddInteraction  (GVec3 pos, float radius, float strength);
    void ClearInteractions();

    // Seasonal tint
    void  SetSeason     (float t);
    GRGB  GetTintColour (float t) const;

    // Tick wind simulation
    void Tick(float dt);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
