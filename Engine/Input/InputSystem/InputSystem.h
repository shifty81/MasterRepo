#pragma once
/**
 * @file InputSystem.h
 * @brief Unified input abstraction for keyboard, mouse, and gamepad.
 *
 * Features:
 *   - Action bindings: map named actions to one or more physical keys/buttons/axes
 *   - Query states: Pressed, Held, Released, Axis value
 *   - Binding maps / contexts: push/pop input contexts to prioritise layers
 *   - Raw events fed from platform layer (OnKey, OnMouseButton, OnMouseMove, OnAxis)
 *   - Axis normalisation and dead-zone handling for gamepad sticks/triggers
 *   - Chord bindings: modifier + key combinations
 *   - Callback-style listeners per action (on-press, on-release, on-axis-change)
 *   - JSON binding definitions with hot-reload
 *
 * Typical usage:
 * @code
 *   InputSystem input;
 *   input.Init();
 *   input.BindAction("Jump",  InputKey::Space);
 *   input.BindAction("Fire",  InputKey::MouseLeft);
 *   input.BindAxis  ("MoveX", InputKey::GamepadLStickX);
 *   input.OnPressed ("Jump",  [](){ player.Jump(); });
 *   // per-frame (before game tick):
 *   input.Tick();
 *   if (input.IsHeld("Sprint")) SprintTick();
 *   float mx = input.GetAxis("MoveX");
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class InputKey : uint32_t {
    // Keyboard
    Unknown=0,
    Space, Enter, Escape, Tab, Backspace, Delete,
    Left, Right, Up, Down,
    A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,
    Num0,Num1,Num2,Num3,Num4,Num5,Num6,Num7,Num8,Num9,
    F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12,
    LeftShift, RightShift, LeftCtrl, RightCtrl, LeftAlt, RightAlt,
    // Mouse
    MouseLeft, MouseRight, MouseMiddle, MouseX1, MouseX2,
    MouseAxisX, MouseAxisY, MouseWheelY,
    // Gamepad
    GamepadA, GamepadB, GamepadX, GamepadY,
    GamepadLBumper, GamepadRBumper, GamepadLTrigger, GamepadRTrigger,
    GamepadStart, GamepadSelect,
    GamepadDPadUp, GamepadDPadDown, GamepadDPadLeft, GamepadDPadRight,
    GamepadLStickX, GamepadLStickY, GamepadRStickX, GamepadRStickY,
    COUNT
};

enum class InputEvent : uint8_t { Pressed=0, Released };

struct ActionBinding {
    std::string       actionName;
    InputKey          key{InputKey::Unknown};
    InputKey          modifier{InputKey::Unknown};  ///< optional chord modifier
};

struct AxisBinding {
    std::string  actionName;
    InputKey     key{InputKey::Unknown};
    float        scale{1.f};  ///< invert or scale raw value
};

class InputSystem {
public:
    InputSystem();
    ~InputSystem();

    void Init();
    void Shutdown();

    /// Call once per frame before game tick to swap state buffers
    void Tick();

    // Raw platform events (call from window/OS callbacks)
    void OnKey         (InputKey key, InputEvent ev);
    void OnMouseButton (InputKey button, InputEvent ev);
    void OnMouseMove   (float dx, float dy);
    void OnMouseWheel  (float dy);
    void OnAxis        (InputKey axis, float value);

    // Binding
    void BindAction(const std::string& name, InputKey key,
                    InputKey modifier=InputKey::Unknown);
    void BindAxis  (const std::string& name, InputKey key, float scale=1.f);
    void UnbindAction(const std::string& name);
    void UnbindAxis  (const std::string& name);

    // State queries
    bool  IsPressed (const std::string& action) const; ///< true exactly one frame
    bool  IsHeld    (const std::string& action) const; ///< true while held
    bool  IsReleased(const std::string& action) const; ///< true exactly one frame on release
    float GetAxis   (const std::string& action) const; ///< normalised [-1,1] or [0,1]

    // Raw key queries (bypass binding)
    bool  KeyPressed (InputKey key) const;
    bool  KeyHeld    (InputKey key) const;
    bool  KeyReleased(InputKey key) const;
    float AxisRaw    (InputKey key) const;

    // Callbacks
    uint32_t OnPressed (const std::string& action, std::function<void()> cb);
    uint32_t OnReleased(const std::string& action, std::function<void()> cb);
    uint32_t OnAxis    (const std::string& action, std::function<void(float)> cb);
    void     RemoveCallback(uint32_t id);

    // Context stack
    void PushContext (const std::string& ctx);
    void PopContext  ();
    void ClearContext();

    // Dead-zone for gamepad axes
    void SetDeadZone(float dz);

    // JSON
    bool LoadBindingsFromJSON(const std::string& path);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
