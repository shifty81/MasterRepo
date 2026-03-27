#pragma once
/**
 * @file CinematicCamera.h
 * @brief Spline-driven cinematic camera with DoF, focal length, cuts, and blending.
 *
 * Features:
 *   - AddKeyframe(time, pos, lookAt, fov, aperture, focusDist): author camera path
 *   - RemoveKeyframe(index)
 *   - EvaluateAt(time) → CameraState: interpolated camera state at given time
 *   - Play(looping): start playback
 *   - Pause() / Stop() / Seek(time)
 *   - Tick(dt): advance internal playhead
 *   - GetCurrentState() → CameraState
 *   - AddCut(atTime): mark a hard cut (no interpolation) at given time
 *   - Blend(fromState, toState, t) → CameraState: static lerp helper
 *   - SetOnCut(cb): callback when a cut point is passed
 *   - SetSplineType(catmullrom/linear/bezier)
 *   - IsPlaying() / GetPlayhead() / GetDuration()
 *   - ExportJSON() / LoadFromJSON()
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct CCVec3 { float x, y, z; };

struct CameraKeyframe {
    float  time;
    CCVec3 pos;
    CCVec3 lookAt;
    float  fovDeg  {60.f};
    float  aperture{2.8f};
    float  focusDist{10.f};
};

struct CameraState {
    CCVec3 pos;
    CCVec3 lookAt;
    float  fovDeg  {60.f};
    float  aperture{2.8f};
    float  focusDist{10.f};
};

enum class SplineType : uint8_t { Linear, CatmullRom, Bezier };

class CinematicCamera {
public:
    CinematicCamera();
    ~CinematicCamera();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Authoring
    uint32_t AddKeyframe   (const CameraKeyframe& kf);
    void     RemoveKeyframe(uint32_t index);
    void     AddCut        (float atTime);
    void     ClearKeyframes();

    // Playback
    void  Play  (bool loop=false);
    void  Pause ();
    void  Stop  ();
    void  Seek  (float time);
    void  Tick  (float dt);

    // Query
    CameraState EvaluateAt     (float time) const;
    CameraState GetCurrentState() const;
    float       GetPlayhead    () const;
    float       GetDuration    () const;
    bool        IsPlaying      () const;
    uint32_t    GetKeyframeCount() const;

    // Static helper
    static CameraState Blend(const CameraState& a, const CameraState& b, float t);

    // Config
    void SetSplineType(SplineType type);
    void SetOnCut     (std::function<void(float time)> cb);

    // Serialisation
    std::string ExportJSON  ()                      const;
    bool        LoadFromJSON(const std::string& json);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
