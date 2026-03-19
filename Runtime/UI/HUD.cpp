#include "Runtime/UI/HUD.h"

namespace Runtime::UI {

void HUD::AddElement(const HUDElement& elem) {
    // Replace if an element with the same id already exists.
    for (auto& e : m_elements) {
        if (e.id == elem.id) {
            e = elem;
            return;
        }
    }
    m_elements.push_back(elem);
}

void HUD::RemoveElement(const std::string& id) {
    auto it = std::find_if(m_elements.begin(), m_elements.end(),
                           [&](const HUDElement& e) { return e.id == id; });
    if (it != m_elements.end())
        m_elements.erase(it);
}

HUDElement* HUD::GetElement(const std::string& id) {
    for (auto& e : m_elements)
        if (e.id == id)
            return &e;
    return nullptr;
}

const HUDElement* HUD::GetElement(const std::string& id) const {
    for (const auto& e : m_elements)
        if (e.id == id)
            return &e;
    return nullptr;
}

void HUD::SetVisible(const std::string& id, bool visible) {
    if (auto* elem = GetElement(id))
        elem->visible = visible;
}

void HUD::Update(float /*dt*/) {
    // Tick any animated HUD elements here (e.g., fade transitions, pulsing).
    // Currently a no-op; extend per element type as needed.
}

void HUD::Render() {
    // Integration point: iterate m_elements and submit draw calls to the UI
    // renderer (e.g., immediate-mode GUI or a custom canvas system).
    // Only visible elements should be rendered:
    //   for (const auto& e : m_elements)
    //       if (e.visible)
    //           UIRenderer::DrawElement(e.type, e.x, e.y, e.width, e.height);
}

} // namespace Runtime::UI
