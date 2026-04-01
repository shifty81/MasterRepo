#include "Input/Devices/Mouse.h"

namespace NF::Input {

Vector2 Mouse::GetPosition() const noexcept {
    return m_Position;
}

Vector2 Mouse::GetDelta() const noexcept {
    return {m_Position.X - m_PreviousPosition.X,
            m_Position.Y - m_PreviousPosition.Y};
}

bool Mouse::IsButtonDown(int button) const noexcept {
    if (button < 0 || button >= kButtonCount) return false;
    return m_Buttons[static_cast<std::size_t>(button)];
}

void Mouse::Update(const Vector2& position,
                   const std::array<bool, kButtonCount>& buttons) noexcept {
    m_PreviousPosition = m_Position;
    m_Position         = position;
    m_Buttons          = buttons;
}

} // namespace NF::Input
