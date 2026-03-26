#pragma once
/**
 * @file InputRemapper.h
 * @brief Runtime input remapping — rebind actions to keys/buttons/axes.
 *
 * Maintains a table of named actions → bindings.  When the engine queries
 * whether an action is pressed/held/released, the remapper translates
 * through the current binding.  Supports:
 *   - Keyboard, mouse button, gamepad button, gamepad axis
 *   - Multiple bindings per action (primary + secondary)
 *   - Conflict detection (warn when two actions share a key)
 *   - Persistence (save/load to JSON)
 *   - Profiles (per-player, per-mode)
 *
 * Typical usage:
 * @code
 *   InputRemapper rm;
 *   rm.Init();
 *   rm.RegisterAction("Jump");
 *   rm.BindKey("Jump", KeyCode::Space);
 *   rm.BindGamepadButton("Jump", GamepadButton::A);
 *   // Remap at runtime:
 *   rm.BindKey("Jump", KeyCode::Enter);
 *   // Query:
 *   bool jumped = rm.IsPressed("Jump");
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class KeyCode : uint32_t {
    Unknown=0,
    Space=32, Enter=13, Escape=27, Tab=9, Backspace=8,
    A=65,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,
    F1=112,F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12,
    Left=37,Up=38,Right=39,Down=40,
    LeftShift=160,RightShift=161,LeftCtrl=162,RightCtrl=163,
    LeftAlt=164,RightAlt=165,
    Num0=48,Num1,Num2,Num3,Num4,Num5,Num6,Num7,Num8,Num9,
};

enum class MouseButton : uint8_t { Left=0, Right=1, Middle=2 };

enum class GamepadButton : uint8_t {
    A=0,B,X,Y,LB,RB,LT,RT,Select,Start,L3,R3,
    DPadUp,DPadDown,DPadLeft,DPadRight,
};

enum class GamepadAxis : uint8_t {
    LeftX=0,LeftY,RightX,RightY,LTrigger,RTrigger,
};

enum class BindingType : uint8_t { Key=0, MouseBtn, GamepadBtn, GamepadAxis };

struct InputBinding {
    BindingType   type{BindingType::Key};
    KeyCode       key{KeyCode::Unknown};
    MouseButton   mouseBtn{MouseButton::Left};
    GamepadButton gpBtn{GamepadButton::A};
    GamepadAxis   gpAxis{GamepadAxis::LeftX};
    float         axisDeadzone{0.15f};
    float         axisScale{1.f};         ///< negative to invert
    bool          isPrimary{true};
};

struct InputAction {
    std::string   name;
    std::vector<InputBinding> bindings;
    bool          isAxis{false};          ///< true for continuous axis actions
};

class InputRemapper {
public:
    InputRemapper();
    ~InputRemapper();

    void Init();
    void Shutdown();

    // Action management
    void RegisterAction(const std::string& name, bool isAxis = false);
    void UnregisterAction(const std::string& name);
    bool HasAction(const std::string& name) const;
    std::vector<InputAction> AllActions() const;

    // Binding
    void BindKey(const std::string& action, KeyCode key, bool primary = true);
    void BindMouseButton(const std::string& action, MouseButton btn, bool primary = true);
    void BindGamepadButton(const std::string& action, GamepadButton btn, bool primary = true);
    void BindGamepadAxis(const std::string& action, GamepadAxis axis,
                         float scale = 1.f, bool primary = true);
    void ClearBindings(const std::string& action);

    // Runtime query (called each frame after Update)
    bool  IsPressed(const std::string& action)  const;
    bool  IsHeld(const std::string& action)     const;
    bool  IsReleased(const std::string& action) const;
    float GetAxis(const std::string& action)    const;

    // Raw state injection (called from platform input layer)
    void SetKeyState(KeyCode key, bool pressed);
    void SetMouseButtonState(MouseButton btn, bool pressed);
    void SetGamepadButtonState(uint8_t pad, GamepadButton btn, bool pressed);
    void SetGamepadAxisValue(uint8_t pad, GamepadAxis axis, float value);
    void Update();   ///< compute pressed/released from prev+current state

    // Conflict detection
    std::vector<std::string> FindConflicts() const;

    // Profile serialization
    bool SaveProfile(const std::string& path) const;
    bool LoadProfile(const std::string& path);
    void ResetToDefaults();

    void OnBindingChanged(std::function<void(const std::string& action)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
