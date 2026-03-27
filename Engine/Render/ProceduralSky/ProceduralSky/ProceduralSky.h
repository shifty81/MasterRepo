#pragma once
/**
 * @file ProceduralSky.h
 * @brief Analytic sky rendering: sun disc, Rayleigh/Mie scattering, moon, star field, horizon.
 *
 * Features:
 *   - SetSunDirection(dir): normalised sun vector (drives scattering)
 *   - SetTimeOfDay(hours): 0–24 advances sun arc along configured latitude
 *   - SetLatitude(degrees): geographic latitude for sun arc computation
 *   - Sample(viewDir) → RGBA: evaluate sky colour for a given view direction
 *   - GetSunColour() → RGB: current sun disc colour (affected by atmosphere)
 *   - GetAmbientColour() → RGB: sky ambient used for lighting
 *   - SetRayleighCoeff(r): Rayleigh scattering coefficient (blue sky intensity)
 *   - SetMieCoeff(m) / SetMieAnisotropy(g): Mie scattering + phase function
 *   - SetExposure(ev): HDR exposure multiplier
 *   - EnableStars(on) / SetStarDensity(n) / SetStarBrightness(b)
 *   - EnableMoon(on) / SetMoonPhase(phase 0–1) / SetMoonDirection(dir)
 *   - SetHorizonFog(colour, density)
 *   - BakeCubemap(outPixels[], faceSize): render full sky cubemap
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <vector>

namespace Engine {

struct SkyRGBA { float r,g,b,a; };
struct SkyRGB  { float r,g,b; };
struct SkyVec3 { float x,y,z; };

class ProceduralSky {
public:
    ProceduralSky();
    ~ProceduralSky();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Primary controls
    void SetSunDirection(SkyVec3 dir);
    void SetTimeOfDay   (float hours);
    void SetLatitude    (float degrees);

    // Atmosphere parameters
    void SetRayleighCoeff(float r);
    void SetMieCoeff     (float m);
    void SetMieAnisotropy(float g);
    void SetExposure     (float ev);

    // Stars
    void EnableStars    (bool on);
    void SetStarDensity (uint32_t n);
    void SetStarBrightness(float b);

    // Moon
    void EnableMoon      (bool on);
    void SetMoonPhase    (float phase);
    void SetMoonDirection(SkyVec3 dir);

    // Fog
    void SetHorizonFog(SkyRGB colour, float density);

    // Sampling
    SkyRGBA Sample         (SkyVec3 viewDir) const;
    SkyRGB  GetSunColour   () const;
    SkyRGB  GetAmbientColour() const;
    SkyVec3 GetSunDirection() const;

    // Cubemap bake
    void BakeCubemap(float* outRGBA, uint32_t faceSize) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
