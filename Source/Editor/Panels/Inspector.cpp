#include "Editor/Panels/Inspector.h"
#include "UI/Rendering/UIRenderer.h"
#include <string>

namespace NF::Editor {

void Inspector::Update([[maybe_unused]] float dt) {}

void Inspector::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;
    (void)w; (void)h;

    static constexpr uint32_t kTextColor  = 0xB0B0B0FF;
    static constexpr uint32_t kLabelColor = 0x808080FF;
    const float scale = 2.f;

    if (m_SelectedEntity == NullEntity || !m_World) {
        m_Renderer->DrawText("No entity selected", x + 6.f, y + 4.f, kLabelColor, scale);
        return;
    }

    std::string label = "Entity " + std::to_string(m_SelectedEntity);
    m_Renderer->DrawText(label, x + 6.f, y + 4.f, kTextColor, scale);
    m_Renderer->DrawText("Components:", x + 6.f, y + 24.f, kLabelColor, scale);
}

} // namespace NF::Editor
