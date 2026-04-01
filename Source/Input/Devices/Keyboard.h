#pragma once
#include <array>
#include <cstdint>

namespace NF::Input {

/// @brief Platform-independent key codes.
enum class KeyCode : uint32_t {
    // Character keys
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Digit row
    Key0, Key1, Key2, Key3, Key4,
    Key5, Key6, Key7, Key8, Key9,

    // Special keys
    Space, Enter, Escape, Backspace, Tab,
    LeftShift, RightShift, LeftCtrl, RightCtrl, LeftAlt, RightAlt,

    // Navigation
    ArrowUp, ArrowDown, ArrowLeft, ArrowRight,
    Home, End, PageUp, PageDown,
    Insert, Delete,

    // Function keys
    F1, F2, F3, F4, F5, F6,
    F7, F8, F9, F10, F11, F12,

    Count ///< Sentinel – keep last.
};

/// @brief Tracks per-frame keyboard state.
class Keyboard {
public:
    /// @brief Returns true while key is held down.
    [[nodiscard]] bool IsKeyDown(KeyCode key) const noexcept;

    /// @brief Returns true only on the first frame key is pressed.
    [[nodiscard]] bool IsKeyPressed(KeyCode key) const noexcept;

    /// @brief Returns true only on the frame key is released.
    [[nodiscard]] bool IsKeyReleased(KeyCode key) const noexcept;

    /// @brief Copy current state to previous and accept new platform input.
    /// @param newState  Bitmask of keys currently held; call once per frame.
    void Update(const std::array<bool, static_cast<std::size_t>(KeyCode::Count)>& newState) noexcept;

private:
    using KeyArray = std::array<bool, static_cast<std::size_t>(KeyCode::Count)>;
    KeyArray m_Current{};
    KeyArray m_Previous{};
};

} // namespace NF::Input
