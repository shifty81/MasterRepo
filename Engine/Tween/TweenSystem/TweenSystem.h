#pragma once
/**
 * @file TweenSystem.h
 * @brief Value interpolation with easing curves, sequences, and completion callbacks.
 *
 * Features:
 *   - Tween float / vec2 / vec3 / vec4 / colour targets
 *   - 26 built-in easing functions (Linear, Quad, Cubic, Sine, Expo, Back, Bounce, Elastic)
 *   - Ease-in / ease-out / ease-in-out variants
 *   - Delay, duration, loop count (finite / infinite), yoyo (ping-pong)
 *   - Sequences: chain tweens to run one-after-another
 *   - Parallel groups: multiple tweens driven by one timeline
 *   - On-complete, on-update, on-loop callbacks
 *   - Pause / resume / kill individual tweens or all tweens on a target
 *   - Time-scale aware (integrates with TimeManager)
 *
 * Typical usage:
 * @code
 *   TweenSystem ts;
 *   ts.Init();
 *   float alpha = 0.f;
 *   uint32_t id = ts.To(&alpha, 1.f, 0.5f, Ease::QuadOut);
 *   ts.OnComplete(id, []{ ShowMenu(); });
 *   // per-frame
 *   ts.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class Ease : uint8_t {
    Linear=0,
    QuadIn, QuadOut, QuadInOut,
    CubicIn, CubicOut, CubicInOut,
    SineIn, SineOut, SineInOut,
    ExpoIn, ExpoOut, ExpoInOut,
    CircIn, CircOut, CircInOut,
    BackIn, BackOut, BackInOut,
    BounceIn, BounceOut, BounceInOut,
    ElasticIn, ElasticOut, ElasticInOut,
    COUNT
};

enum class LoopMode : uint8_t { None=0, Restart, Yoyo };

struct TweenDesc {
    float*    target{nullptr};      ///< pointer(s) to value(s) — up to 4 floats
    uint32_t  numComponents{1};
    float     from[4]{};
    float     to[4]{};
    float     duration{1.f};
    float     delay{0.f};
    Ease      ease{Ease::Linear};
    LoopMode  loop{LoopMode::None};
    int32_t   loopCount{0};         ///< 0 = infinite when loop!=None
    bool      useFrom{false};       ///< if false, capture current value at start
};

class TweenSystem {
public:
    TweenSystem();
    ~TweenSystem();

    void Init();
    void Shutdown();
    void Tick(float dt);

    // Single-float shortcuts
    uint32_t To(float* target, float toVal, float duration, Ease ease=Ease::Linear);
    uint32_t From(float* target, float fromVal, float duration, Ease ease=Ease::Linear);

    // Multi-component (vec3 etc.)
    uint32_t ToVec(float* target, const float* toVals, uint32_t n,
                   float duration, Ease ease=Ease::Linear);

    // Full control
    uint32_t Play(const TweenDesc& desc);

    // Modifiers (call after Play, before first Tick)
    TweenSystem& Delay(uint32_t id, float delay);
    TweenSystem& Loop(uint32_t id, LoopMode mode, int32_t count=0);
    TweenSystem& OnComplete(uint32_t id, std::function<void()> cb);
    TweenSystem& OnUpdate(uint32_t id, std::function<void(float progress)> cb);

    // Control
    void Pause(uint32_t id);
    void Resume(uint32_t id);
    void Kill(uint32_t id);
    void KillAll(float* target=nullptr);
    bool IsAlive(uint32_t id) const;

    // Sequences
    uint32_t CreateSequence();
    void     SequenceAppend(uint32_t seqId, uint32_t tweenId);
    void     SequencePlay(uint32_t seqId);

    // Easing utility
    static float Evaluate(Ease ease, float t); ///< t in [0,1] → eased [0,1]

    uint32_t ActiveCount() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
