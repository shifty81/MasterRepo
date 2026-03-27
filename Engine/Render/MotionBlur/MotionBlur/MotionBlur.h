#pragma once
/**
 * @file MotionBlur.h
 * @brief Per-object velocity-based motion blur: velocity buffer, gather/scatter blur.
 *
 * Features:
 *   - SetShutterAngle(degrees): controls blur extent (0-360)
 *   - SetMaxSamples(n): quality/cost trade-off
 *   - SetMaxBlurRadius(px): screen-space clamp
 *   - RegisterObject(objId, prevMVP, currMVP): per-object matrix pair
 *   - UnregisterObject(objId)
 *   - ComputeVelocityBuffer(width, height, outVel[]): write per-pixel screen velocity
 *   - Apply(width, height, inColor[], inVel[], outColor[]): gather blur pass
 *   - SetEnabled(on) / IsEnabled() → bool
 *   - SetCameraBlend(v): fraction of camera motion to include [0,1]
 *   - SetOnApply(cb): callback(width, height) before each Apply()
 *   - Clear(): remove all registered objects
 *   - Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct MBMat4 { float m[16]; };

class MotionBlur {
public:
    MotionBlur();
    ~MotionBlur();

    void Init    ();
    void Shutdown();
    void Clear   ();

    // Config
    void SetShutterAngle (float degrees);
    void SetMaxSamples   (uint32_t n);
    void SetMaxBlurRadius(float px);
    void SetCameraBlend  (float v);
    void SetEnabled      (bool on);
    bool IsEnabled       () const;

    // Object registration
    void RegisterObject  (uint32_t objId, const MBMat4& prevMVP, const MBMat4& currMVP);
    void UnregisterObject(uint32_t objId);

    // Processing
    void ComputeVelocityBuffer(uint32_t w, uint32_t h,
                               std::vector<float>& outVel) const;
    void Apply(uint32_t w, uint32_t h,
               const std::vector<float>& inColor,
               const std::vector<float>& inVel,
               std::vector<float>&       outColor) const;

    // Callback
    void SetOnApply(std::function<void(uint32_t w, uint32_t h)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
