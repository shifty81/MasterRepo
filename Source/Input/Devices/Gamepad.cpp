#include "Input/Devices/Gamepad.h"
#include <algorithm>
#include <cmath>

namespace NF::Input {

namespace {
    /// @brief Apply a symmetric dead-zone to a stick axis.
    constexpr float ApplyDeadzone(float v, float dz = 0.1f) noexcept {
        if (v > dz)  return (v - dz) / (1.f - dz);
        if (v < -dz) return (v + dz) / (1.f - dz);
        return 0.f;
    }
} // anonymous namespace

bool Gamepad::IsButtonDown(GamepadButton button) const noexcept {
    const auto idx = static_cast<std::size_t>(button);
    return idx < m_Current.size() && m_Current[idx];
}

bool Gamepad::IsButtonPressed(GamepadButton button) const noexcept {
    const auto idx = static_cast<std::size_t>(button);
    return idx < m_Current.size() && m_Current[idx] && !m_Previous[idx];
}

bool Gamepad::IsButtonReleased(GamepadButton button) const noexcept {
    const auto idx = static_cast<std::size_t>(button);
    return idx < m_Current.size() && !m_Current[idx] && m_Previous[idx];
}

void Gamepad::Update(const State& newState) noexcept {
    m_Previous  = m_Current;
    m_Current   = newState.Buttons;
    m_Connected = newState.Connected;

    // Apply dead-zones to stick axes
    m_LeftStick  = { ApplyDeadzone(newState.LeftStick.X),
                     ApplyDeadzone(newState.LeftStick.Y) };
    m_RightStick = { ApplyDeadzone(newState.RightStick.X),
                     ApplyDeadzone(newState.RightStick.Y) };

    // Clamp triggers to [0, 1]
    m_LeftTrigger  = std::clamp(newState.LeftTrigger,  0.f, 1.f);
    m_RightTrigger = std::clamp(newState.RightTrigger, 0.f, 1.f);
}

} // namespace NF::Input
