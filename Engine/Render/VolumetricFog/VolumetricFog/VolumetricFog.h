#pragma once
/**
 * @file VolumetricFog.h
 * @brief CPU raymarched volumetric fog with density volume, scattering, and directional light shaft.
 *
 * Features:
 *   - Init(volumeW, volumeH, volumeD): 3D density grid (float, row-major XYZ)
 *   - SetDensity(x, y, z, d): write density at voxel
 *   - FillBox(minX,minY,minZ,maxX,maxY,maxZ,density): bulk fill
 *   - SetGlobalDensity(d): uniform base density
 *   - RaymarchFog(origin, dir, tMin, tMax, steps) → FogSample: accumulated colour+opacity
 *   - SetScatterColour(r,g,b): in-scatter light colour
 *   - SetAbsorption(a): beer-law extinction coefficient
 *   - SetLightDir(x,y,z): directional light direction for Mie/Henyey-Greenstein phase
 *   - SetPhaseG(g): HG asymmetry parameter [-1,1]
 *   - AnimateDensity(windX, windY, windZ, dt): advect density field (periodic wrapping)
 *   - Tick(dt): advance internal time for animated effects
 *   - Reset() / Shutdown()
 */

#include <cstdint>

namespace Engine {

struct FogVec3 { float x,y,z; };

struct FogSample {
    float r,g,b;    ///< accumulated inscatter colour
    float alpha;    ///< accumulated opacity [0,1]
    float depth;    ///< mean free-path depth
};

class VolumetricFog {
public:
    VolumetricFog();
    ~VolumetricFog();

    void Init    (uint32_t vw, uint32_t vh, uint32_t vd, float worldScale=1.f);
    void Shutdown();
    void Reset   ();
    void Tick    (float dt);

    // Density volume authoring
    void  SetDensity    (uint32_t x, uint32_t y, uint32_t z, float d);
    float GetDensity    (uint32_t x, uint32_t y, uint32_t z) const;
    void  FillBox       (float minX,float minY,float minZ,
                         float maxX,float maxY,float maxZ, float density);
    void  SetGlobalDensity(float d);

    // Raymarching
    FogSample RaymarchFog(FogVec3 origin, FogVec3 dir,
                          float tMin, float tMax, uint32_t steps=32) const;

    // Lighting
    void SetScatterColour(float r, float g, float b);
    void SetAbsorption   (float a);
    void SetLightDir     (float x, float y, float z);
    void SetPhaseG       (float g); ///< HG asymmetry, 0=isotropic

    // Animation
    void AnimateDensity(float windX, float windY, float windZ, float dt);

    uint32_t VolumeW() const;
    uint32_t VolumeH() const;
    uint32_t VolumeD() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
