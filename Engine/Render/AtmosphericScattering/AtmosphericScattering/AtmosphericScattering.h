#pragma once
/**
 * @file AtmosphericScattering.h
 * @brief Rayleigh+Mie sky: sun direction, precomputed LUT, CPU sky-colour evaluation.
 *
 * Features:
 *   - AtmosphericParams: Rayleigh/Mie coefficients, scaleHeight, earthRadius, atmosRadius
 *   - Sun direction + intensity
 *   - BuildLUT(width, height): precompute 2D sky LUT (view-zenith x sun-zenith angles)
 *   - EvaluateSkyColour(viewDir) → RGB via LUT lookup + interpolation
 *   - EvaluateSkyColourDirect(viewDir) → RGB via numerical integration (no LUT)
 *   - GetSunColour() → RGB attenuated sun disc colour
 *   - GetHorizonColour() → average horizon band colour
 *   - SetTime(hours): derive sun elevation from time-of-day
 *   - LUT raw data pointer for GPU upload (float RGB, row-major)
 *   - IsDirty() / ClearDirty()
 *
 * Typical usage:
 * @code
 *   AtmosphericScattering sky;
 *   sky.Init();
 *   sky.SetTime(14.5f); // 2:30 PM
 *   sky.BuildLUT(128, 64);
 *   float col[3]; sky.EvaluateSkyColour({0.3f, 0.8f, 0.5f}, col);
 * @endcode
 */

#include <cstdint>
#include <vector>

namespace Engine {

struct AtmosphericParams {
    float rayleighCoeff[3]{5.8e-6f, 13.5e-6f, 33.1e-6f};
    float mieCoeff      {21e-6f};
    float rayleighScaleH{8500.f};   ///< metres
    float mieScaleH     {1200.f};
    float mieG          {0.76f};    ///< Henyey-Greenstein asymmetry
    float earthRadius   {6360000.f};
    float atmosRadius   {6420000.f};
    float sunIntensity  {20.f};
};

class AtmosphericScattering {
public:
    AtmosphericScattering();
    ~AtmosphericScattering();

    void Init    ();
    void Shutdown();

    void SetParams   (const AtmosphericParams& p);
    const AtmosphericParams& GetParams() const;

    // Sun
    void SetSunDirection(const float dir[3]);   ///< world-space unit vector toward sun
    void GetSunDirection(float out[3]) const;
    void SetTime(float hoursUTC);               ///< 0-24, auto-derive sun elevation

    // LUT
    void BuildLUT    (uint32_t w=128, uint32_t h=64);
    bool HasLUT      () const;
    uint32_t LUTWidth () const;
    uint32_t LUTHeight() const;
    const float* LUTData() const; ///< w*h*3 floats RGB

    // Evaluate
    void EvaluateSkyColour      (const float viewDir[3], float outRGB[3]) const; ///< uses LUT
    void EvaluateSkyColourDirect(const float viewDir[3], float outRGB[3]) const; ///< integration
    void GetSunColour           (float outRGB[3]) const;
    void GetHorizonColour       (float outRGB[3]) const;

    // Dirty state
    bool IsDirty  () const;
    void ClearDirty();

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
