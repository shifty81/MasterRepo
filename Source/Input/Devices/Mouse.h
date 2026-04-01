#pragma once
#include "Core/Math/Vector.h"
#include <array>

namespace NF::Input {

/// @brief Tracks per-frame mouse state.
class Mouse {
public:
    static constexpr int kButtonCount = 8;

    /// @brief Current cursor position in screen pixels.
    [[nodiscard]] Vector2 GetPosition() const noexcept;

    /// @brief Cursor displacement since the previous Update call.
    [[nodiscard]] Vector2 GetDelta() const noexcept;

    /// @brief Returns true while a mouse button is held.
    /// @param button  Zero-based button index (0 = left, 1 = right, 2 = middle).
    [[nodiscard]] bool IsButtonDown(int button) const noexcept;

    /// @brief Provide a new platform snapshot to the mouse.
    /// @param position  Current cursor position in screen pixels.
    /// @param buttons   Held state for each button.
    void Update(const Vector2& position,
                const std::array<bool, kButtonCount>& buttons) noexcept;

private:
    Vector2 m_Position{};
    Vector2 m_PreviousPosition{};
    std::array<bool, kButtonCount> m_Buttons{};
};

} // namespace NF::Input
