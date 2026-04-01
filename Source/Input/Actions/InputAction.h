#pragma once
#include "Input/Devices/Keyboard.h"
#include "Input/Devices/Mouse.h"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <variant>

namespace NF::Input {

/// @brief Classifies what kind of value an action produces.
enum class ActionType {
    Button, ///< Boolean (pressed / released).
    Axis1D, ///< Single float value.
    Axis2D  ///< Two-dimensional float vector.
};

/// @brief A logical input action that can be bound to one or more keys.
class InputAction {
public:
    /// @brief Displayable/lookup name for this action.
    std::string Name;

    /// @brief How this action's value should be interpreted.
    ActionType Type{ActionType::Button};

    /// @brief Keys bound to this action.
    std::vector<KeyCode> Bindings;

    /// @brief Retrieve the current value of this action.
    ///
    /// Supported specialisations:
    ///   - bool   – true while any bound key is held (Button)
    ///   - float  – 1.0f while any bound key is held (Axis1D)
    ///   - Vector2 – (1,0) for the first bound key, etc. (Axis2D)
    template<typename T>
    [[nodiscard]] T GetValue(const Keyboard& kb) const;

private:
    template<typename T>
    T GetValueImpl(const Keyboard& kb) const;
};

/// @brief Manages a set of InputAction objects and their key bindings.
class InputSystem {
public:
    /// @brief Register an action with the system.
    void RegisterAction(InputAction action);

    /// @brief Bind an additional key to a named action.
    void BindKey(std::string_view actionName, KeyCode key);

    /// @brief Update all actions from the provided device states.
    void Update(Keyboard& keyboard, Mouse& mouse);

    /// @brief Look up a registered action by name.
    /// @return Pointer to the action, or nullptr if not found.
    [[nodiscard]] const InputAction* FindAction(std::string_view name) const;

private:
    std::vector<InputAction> m_Actions;
};

// ---------------------------------------------------------------------------
// Template implementation
// ---------------------------------------------------------------------------

template<>
inline bool InputAction::GetValue<bool>(const Keyboard& kb) const {
    for (const KeyCode key : Bindings)
        if (kb.IsKeyDown(key)) return true;
    return false;
}

template<>
inline float InputAction::GetValue<float>(const Keyboard& kb) const {
    return GetValue<bool>(kb) ? 1.f : 0.f;
}

template<>
inline Vector2 InputAction::GetValue<Vector2>(const Keyboard& kb) const {
    for (const KeyCode key : Bindings)
        if (kb.IsKeyDown(key)) return {1.f, 0.f};
    return {0.f, 0.f};
}

} // namespace NF::Input
