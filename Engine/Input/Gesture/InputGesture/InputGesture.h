#pragma once
/**
 * @file InputGesture.h
 * @brief Touch/pointer gesture recogniser: tap, double-tap, swipe, pinch, rotate, long-press.
 *
 * Features:
 *   - Feed: BeginTouch(id, x, y, time) / MoveTouch(id, x, y, time) / EndTouch(id, x, y, time)
 *   - Recognised gestures: Tap, DoubleTap, LongPress, SwipeLeft/Right/Up/Down,
 *                          PinchZoom(scale), RotateGesture(angle), Pan(dx,dy)
 *   - GestureEvent: type, position, velocity, scale, angle, duration
 *   - SetOnGesture(callback)
 *   - Configurable: tapMaxDuration, doubleTapMaxInterval, swipeMinDistance,
 *                   longPressMinDuration, pinchMinScale
 *   - Multi-touch: up to 5 simultaneous touch points
 *   - CancelGesture(): reset state
 *   - IsGestureInProgress() → bool
 */

#include <cstdint>
#include <functional>
#include <string>

namespace Engine {

enum class GestureType : uint8_t {
    None=0,
    Tap, DoubleTap, LongPress,
    SwipeLeft, SwipeRight, SwipeUp, SwipeDown,
    Pinch, Rotate, Pan
};

struct GestureEvent {
    GestureType type   {GestureType::None};
    float       x      {0}, y{0};       ///< position (centre for multi-touch)
    float       dx     {0}, dy{0};      ///< delta for Pan/Swipe
    float       scale  {1.f};           ///< for Pinch
    float       angle  {0.f};           ///< radians for Rotate
    float       duration{0.f};          ///< for LongPress/Tap
    uint32_t    touchCount{1};
};

struct GestureConfig {
    float tapMaxDuration      {0.25f};  ///< seconds
    float doubleTapMaxInterval{0.35f};
    float swipeMinDistance    {40.f};   ///< pixels
    float longPressMinDuration{0.5f};
    float pinchMinScale       {0.05f};  ///< minimum scale change
    float panMinDistance      {5.f};
};

class InputGesture {
public:
    InputGesture();
    ~InputGesture();

    void Init    (const GestureConfig& cfg = {});
    void Shutdown();
    void Tick    (float dt);

    // Touch feed
    void BeginTouch(uint32_t id, float x, float y, float time);
    void MoveTouch (uint32_t id, float x, float y, float time);
    void EndTouch  (uint32_t id, float x, float y, float time);
    void CancelGesture();

    // Config
    void SetConfig(const GestureConfig& cfg);
    const GestureConfig& GetConfig() const;

    // Callback
    void SetOnGesture(std::function<void(const GestureEvent&)> cb);

    // State
    bool IsGestureInProgress() const;
    uint32_t ActiveTouchCount() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
