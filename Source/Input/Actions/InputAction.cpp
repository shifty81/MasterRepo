#include "Input/Actions/InputAction.h"
#include <algorithm>

namespace NF::Input {

void InputSystem::RegisterAction(InputAction action) {
    m_Actions.push_back(std::move(action));
}

void InputSystem::BindKey(std::string_view actionName, KeyCode key) {
    auto it = std::find_if(m_Actions.begin(), m_Actions.end(),
                           [actionName](const InputAction& a) { return a.Name == actionName; });
    if (it != m_Actions.end())
        it->Bindings.push_back(key);
}

void InputSystem::Update(Keyboard& /*keyboard*/, Mouse& /*mouse*/) {
    // Per-frame processing hook; per-device Update() is called externally.
}

const InputAction* InputSystem::FindAction(std::string_view name) const {
    auto it = std::find_if(m_Actions.begin(), m_Actions.end(),
                           [name](const InputAction& a) { return a.Name == name; });
    return (it != m_Actions.end()) ? &*it : nullptr;
}

} // namespace NF::Input
