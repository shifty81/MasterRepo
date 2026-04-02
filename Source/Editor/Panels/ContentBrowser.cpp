#include "Editor/Panels/ContentBrowser.h"
#include "UI/Rendering/UIRenderer.h"

namespace NF::Editor {

void ContentBrowser::Update([[maybe_unused]] float dt) {}

void ContentBrowser::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;

    static constexpr uint32_t kTextColor    = 0xB0B0B0FF;
    static constexpr uint32_t kLabelColor   = 0x808080FF;
    static constexpr uint32_t kHoverColor   = 0x3F3F50FF;
    static constexpr uint32_t kSelectColor  = 0x264F78FF;

    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 18.f * dpi;
    const float padX  = 6.f  * dpi;
    const float scale = 2.f;

    // Helper: draw one clickable row and return whether it was clicked.
    float cy = y + 4.f * dpi;

    // Root path label (non-clickable header)
    const std::string rootLabel = "Root: " + (m_RootPath.empty() ? "(none)" : m_RootPath);
    m_Renderer->DrawText(rootLabel, x + padX, cy, kLabelColor, scale);
    cy += lineH;

    // Entries — simple static list matching the actual content structure.
    const char* kEntries[] = {
        "Assets/",
        "  Definitions/",
        "  Textures/",
        "  Models/",
    };

    for (const char* entry : kEntries) {
        if (cy + lineH > y + h) break;

        const std::string entryStr(entry);
        const bool hovered  = m_Input &&
                              m_Input->mouseX >= x          && m_Input->mouseX < x + w &&
                              m_Input->mouseY >= cy         && m_Input->mouseY < cy + lineH;
        const bool selected = (m_SelectedAsset == entryStr);

        if (selected) {
            m_Renderer->DrawRect({x, cy, w, lineH}, kSelectColor);
        } else if (hovered) {
            m_Renderer->DrawRect({x, cy, w, lineH}, kHoverColor);
        }

        if (hovered && m_Input->leftJustPressed) {
            m_SelectedAsset = entryStr;
        }

        const uint32_t rowColor = selected ? 0xFFFFFFFF : kTextColor;
        m_Renderer->DrawText(entryStr, x + padX, cy + 2.f * dpi, rowColor, scale);
        cy += lineH;
    }
}

} // namespace NF::Editor
