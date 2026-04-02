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
    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 20.f * dpi;
    const float padX  = 6.f * dpi;
    const float scale = 2.f;

    if (m_SelectedEntity == NullEntity || !m_World) {
        m_Renderer->DrawText("No entity selected", x + padX, y + 4.f * dpi, kLabelColor, scale);
        return;
    }

    std::string label = "Entity " + std::to_string(m_SelectedEntity);
    m_Renderer->DrawText(label,        x + padX, y + 4.f  * dpi,           kTextColor,  scale);
    m_Renderer->DrawText("Components:", x + padX, y + 4.f  * dpi + lineH,  kLabelColor, scale);
}

} // namespace NF::Editor
