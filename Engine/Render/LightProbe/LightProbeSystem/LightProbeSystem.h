#pragma once
/**
 * @file LightProbeSystem.h
 * @brief SH-based light probe capture, interpolation, and diffuse irradiance sampling.
 *
 * Features:
 *   - PlaceProbe(probeId, x, y, z) → bool
 *   - RemoveProbe(probeId)
 *   - CaptureProbe(probeId, radiance[6][size²]): load 6-face cubemap
 *   - ProjectToSH(probeId): compute L2 spherical harmonics coefficients
 *   - SampleIrradiance(probeId, nx, ny, nz, outRGB[3]): evaluate SH for normal
 *   - InterpolateIrradiance(x, y, z, nx, ny, nz, outRGB[3]): blend nearest probes
 *   - SetBlendRadius(r): max blend distance between probes
 *   - GetProbeCount() → uint32_t
 *   - GetProbePosition(probeId, outX, outY, outZ)
 *   - GetSHCoefficients(probeId, out[27]): 9 L2 SH × RGB
 *   - SetOnCapture(cb): callback(probeId) after capture+project
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

class LightProbeSystem {
public:
    LightProbeSystem();
    ~LightProbeSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Probe management
    bool PlaceProbe (uint32_t probeId, float x, float y, float z);
    void RemoveProbe(uint32_t probeId);

    // Capture & SH projection
    void CaptureProbe  (uint32_t probeId, const float* radiance,
                        uint32_t faceSize);
    void ProjectToSH   (uint32_t probeId);

    // Sampling
    void SampleIrradiance      (uint32_t probeId,
                                float nx, float ny, float nz,
                                float outRGB[3]) const;
    void InterpolateIrradiance (float x, float y, float z,
                                float nx, float ny, float nz,
                                float outRGB[3]) const;

    // Config
    void SetBlendRadius(float r);

    // Queries
    uint32_t GetProbeCount    () const;
    void     GetProbePosition (uint32_t probeId,
                               float& outX, float& outY, float& outZ) const;
    void     GetSHCoefficients(uint32_t probeId, float out[27]) const;

    // Callback
    void SetOnCapture(std::function<void(uint32_t probeId)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
