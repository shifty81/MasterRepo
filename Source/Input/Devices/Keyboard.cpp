#include "Input/Devices/Keyboard.h"

namespace NF::Input {

bool Keyboard::IsKeyDown(KeyCode key) const noexcept {
    return m_Current[static_cast<std::size_t>(key)];
}

bool Keyboard::IsKeyPressed(KeyCode key) const noexcept {
    const auto idx = static_cast<std::size_t>(key);
    return m_Current[idx] && !m_Previous[idx];
}

bool Keyboard::IsKeyReleased(KeyCode key) const noexcept {
    const auto idx = static_cast<std::size_t>(key);
    return !m_Current[idx] && m_Previous[idx];
}

void Keyboard::Update(const KeyArray& newState) noexcept {
    m_Previous = m_Current;
    m_Current  = newState;
}

} // namespace NF::Input
