#include "Editor/Panels/SceneOutliner.h"
#include "UI/Rendering/UIRenderer.h"
#include <string>

namespace NF::Editor {

void SceneOutliner::Update([[maybe_unused]] float dt) {}

void SceneOutliner::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;

    static constexpr uint32_t kTextColor   = 0xB0B0B0FF;
    static constexpr uint32_t kLabelColor  = 0x808080FF;
    static constexpr uint32_t kHoverColor  = 0x3F3F50FF;  // subtle blue-grey hover
    static constexpr uint32_t kSelectColor = 0x264F78FF;  // VS-style selection blue

    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 18.f * dpi;
    const float scale = 2.f;               // UIRenderer multiplies by DPI
    float cy = y + 4.f * dpi;

    if (!m_World) {
        m_Renderer->DrawText("No world loaded", x + 6.f * dpi, cy, kLabelColor, scale);
        return;
    }

    // List entities from the ECS world
    const auto& entities = m_World->GetLiveEntities();
    if (entities.empty()) {
        m_Renderer->DrawText("(empty world)", x + 6.f * dpi, cy, kLabelColor, scale);
        return;
    }

    for (EntityId e : entities) {
        if (cy + lineH > y + h) break; // clip to panel

        const bool hovered  = m_Input &&
                              m_Input->mouseX >= x           && m_Input->mouseX < x + w &&
                              m_Input->mouseY >= cy          && m_Input->mouseY < cy + lineH;
        const bool selected = (e == m_SelectedEntity);

        // Draw row background for selected / hovered states
        if (selected) {
            m_Renderer->DrawRect({x, cy, w, lineH}, kSelectColor);
        } else if (hovered) {
            m_Renderer->DrawRect({x, cy, w, lineH}, kHoverColor);
        }

        // Click to select entity
        if (hovered && m_Input->leftJustPressed) {
            m_SelectedEntity = e;
            if (m_OnSelectionChanged) m_OnSelectionChanged(e);
        }

        const uint32_t rowTextColor = selected ? 0xFFFFFFFF : kTextColor;
        std::string label = "Entity " + std::to_string(e);
        m_Renderer->DrawText(label, x + 6.f * dpi, cy + 2.f * dpi, rowTextColor, scale);
        cy += lineH;
    }
}

} // namespace NF::Editor
