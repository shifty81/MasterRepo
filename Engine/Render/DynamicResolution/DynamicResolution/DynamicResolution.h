#pragma once
/**
 * @file DynamicResolution.h
 * @brief Render resolution scale: target FPS, history buffer, scale up/down rules.
 *
 * Features:
 *   - Target frame time (e.g. 16.67ms for 60fps)
 *   - Frame-time history buffer (configurable length, default 8 frames)
 *   - Scale-up: if rolling-average < targetFrameTime * upThreshold → increase scale
 *   - Scale-down: if any frame > targetFrameTime * downThreshold → decrease scale
 *   - Min/max scale clamps (default 0.5 – 1.0)
 *   - Step size for scale adjustments (default 0.05)
 *   - GetCurrentScale() → float [minScale, maxScale]
 *   - GetResolution(baseW, baseH) → (scaledW, scaledH)
 *   - Tick(actualFrameTime): feed frame time each frame
 *   - ForceScale(f): override, clears history
 *   - On-scale-changed callback
 *   - Reset()
 *
 * Typical usage:
 * @code
 *   DynamicResolution dr;
 *   dr.Init(1920, 1080, 60.f);
 *   dr.SetScaleRange(0.5f, 1.0f);
 *   dr.Tick(frameTime);
 *   auto [w,h] = dr.GetResolution();
 *   SetRenderTargetSize(w, h);
 * @endcode
 */

#include <cstdint>
#include <functional>

namespace Engine {

class DynamicResolution {
public:
    DynamicResolution();
    ~DynamicResolution();

    void Init    (uint32_t baseW=1920, uint32_t baseH=1080, float targetFPS=60.f);
    void Shutdown();

    void Tick(float actualFrameTimeSeconds);

    // Scale range
    void  SetScaleRange(float minScale, float maxScale);
    float GetMinScale() const;
    float GetMaxScale() const;

    // Scale step
    void  SetScaleStep(float step);
    float GetScaleStep() const;

    // Thresholds
    void  SetUpThreshold  (float t);  ///< fraction below target before scaling up (default 0.85)
    void  SetDownThreshold(float t);  ///< fraction above target before scaling down (default 1.05)

    // History
    void     SetHistoryLength(uint32_t frames);
    uint32_t GetHistoryLength() const;

    // Current scale
    float    GetCurrentScale()  const;
    void     ForceScale(float s);
    void     Reset();

    // Resolution
    void GetResolution(uint32_t& outW, uint32_t& outH) const;

    // Callback
    void SetOnScaleChanged(std::function<void(float oldScale, float newScale)> cb);

    // Stats
    float AverageFrameTime() const;
    float WorstFrameTime()   const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
