#pragma once
/**
 * @file CurveEditor.h
 * @brief Runtime curve/spline data: key management, tangent modes, evaluate.
 *
 * Features:
 *   - Curve key: time, value, in/out tangent
 *   - Tangent modes per key: Auto, Flat, Free, Broken, Linear, Constant
 *   - Hermite cubic interpolation between keys
 *   - Per-curve WrapMode: Clamp, Loop, PingPong
 *   - Multi-channel curve (e.g. R, G, B, A separately)
 *   - Evaluate(t) → float (single channel)
 *   - EvaluateVec4(t, out[4]) for multi-channel
 *   - Key add/remove/move operations
 *   - Auto-tangent smoothing
 *   - Bake to uniform sample array for runtime perf
 *   - JSON serialise/deserialise
 *
 * Typical usage:
 * @code
 *   CurveEditor curve;
 *   curve.AddKey(0.f, 0.f);
 *   curve.AddKey(0.5f, 1.f, TangentMode::Auto);
 *   curve.AddKey(1.f, 0.f);
 *   float v = curve.Evaluate(0.25f);  // ~0.75
 *   curve.SetWrapMode(WrapMode::Loop);
 *   float v2 = curve.Evaluate(1.3f);  // wraps to 0.3
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class TangentMode : uint8_t { Auto=0, Flat, Free, Broken, Linear, Constant };
enum class WrapMode    : uint8_t { Clamp=0, Loop, PingPong };

struct CurveKey {
    float       time   {0.f};
    float       value  {0.f};
    float       inTan  {0.f};
    float       outTan {0.f};
    TangentMode mode   {TangentMode::Auto};
};

class CurveEditor {
public:
    CurveEditor();
    ~CurveEditor();

    // Key management
    uint32_t AddKey   (float t, float v, TangentMode mode=TangentMode::Auto);
    void     RemoveKey(uint32_t keyIndex);
    void     MoveKey  (uint32_t keyIndex, float newT, float newV);
    void     SetTangent(uint32_t keyIndex, float inTan, float outTan);
    void     SetTangentMode(uint32_t keyIndex, TangentMode mode);
    uint32_t KeyCount () const;
    const CurveKey* GetKey(uint32_t index) const;

    // Wrap
    void     SetWrapMode(WrapMode mode);
    WrapMode GetWrapMode() const;

    // Evaluate
    float Evaluate(float t) const;
    void  Bake    (uint32_t samples, std::vector<float>& outValues) const;

    // Time range
    float StartTime() const;
    float EndTime  () const;

    // Serialise
    std::string Serialize  () const;
    bool        Deserialize(const std::string& json);

    void Clear();
    void SmoothTangents();

    // Multi-channel helpers (channel index 0-3)
    // CurveEditor holds up to 4 channels internally
    CurveEditor& Channel(uint32_t ch);
    void EvaluateVec4(float t, float out[4]) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
