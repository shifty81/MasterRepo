#pragma once
#include "Core/Math/Vector.h"
#include <array>
#include <cstdint>

namespace NF::Input {

/// @brief Buttons present on a standard gamepad / controller.
enum class GamepadButton : uint32_t {
    // Face buttons
    South,   ///< Cross / A
    East,    ///< Circle / B
    West,    ///< Square / X
    North,   ///< Triangle / Y

    // Shoulder / trigger buttons
    LeftBumper,
    RightBumper,
    LeftTriggerButton,   ///< L2 pressed as digital button
    RightTriggerButton,  ///< R2 pressed as digital button

    // Stick clicks
    LeftStick,
    RightStick,

    // D-Pad
    DPadUp, DPadDown, DPadLeft, DPadRight,

    // Menu
    Start, Select, Home,

    Count ///< Sentinel – keep last.
};

/// @brief Per-frame gamepad / controller state.
///
/// Call Update() once per frame with the platform-supplied state to get
/// correct pressed/released edge events.
class Gamepad {
public:
    // -----------------------------------------------------------------
    // Digital buttons
    // -----------------------------------------------------------------

    /// @brief Returns true while @p button is held down.
    [[nodiscard]] bool IsButtonDown(GamepadButton button) const noexcept;

    /// @brief Returns true only on the first frame @p button is pressed.
    [[nodiscard]] bool IsButtonPressed(GamepadButton button) const noexcept;

    /// @brief Returns true only on the frame @p button is released.
    [[nodiscard]] bool IsButtonReleased(GamepadButton button) const noexcept;

    // -----------------------------------------------------------------
    // Analog axes (values in [-1, 1] for sticks, [0, 1] for triggers)
    // -----------------------------------------------------------------

    /// @brief Left stick X/Y axis, each in [-1, 1].
    [[nodiscard]] Vector2 GetLeftStick()  const noexcept { return m_LeftStick;  }

    /// @brief Right stick X/Y axis, each in [-1, 1].
    [[nodiscard]] Vector2 GetRightStick() const noexcept { return m_RightStick; }

    /// @brief Left trigger value in [0, 1].
    [[nodiscard]] float GetLeftTrigger()  const noexcept { return m_LeftTrigger;  }

    /// @brief Right trigger value in [0, 1].
    [[nodiscard]] float GetRightTrigger() const noexcept { return m_RightTrigger; }

    // -----------------------------------------------------------------
    // Connection
    // -----------------------------------------------------------------

    /// @brief Returns true when a controller is connected.
    [[nodiscard]] bool IsConnected() const noexcept { return m_Connected; }

    // -----------------------------------------------------------------
    // Frame update
    // -----------------------------------------------------------------

    /// @brief State fed to Update() each frame.
    struct State {
        std::array<bool, static_cast<std::size_t>(GamepadButton::Count)> Buttons{};
        Vector2 LeftStick{};
        Vector2 RightStick{};
        float   LeftTrigger{0.f};
        float   RightTrigger{0.f};
        bool    Connected{false};
    };

    /// @brief Advance one frame with the supplied platform state.
    void Update(const State& newState) noexcept;

private:
    using ButtonArray = std::array<bool, static_cast<std::size_t>(GamepadButton::Count)>;

    ButtonArray m_Current{};
    ButtonArray m_Previous{};

    Vector2 m_LeftStick{};
    Vector2 m_RightStick{};
    float   m_LeftTrigger{0.f};
    float   m_RightTrigger{0.f};
    bool    m_Connected{false};
};

} // namespace NF::Input
