#include "Editor/Panels/ConsolePanel.h"
#include "UI/Rendering/UIRenderer.h"

namespace NF::Editor {

void ConsolePanel::Update([[maybe_unused]] float dt) {}

void ConsolePanel::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;
    (void)w;

    static constexpr uint32_t kTextColor  = 0x9CDCFEFF; // light cyan
    static constexpr uint32_t kLabelColor = 0x808080FF;
    const float lineH = 18.f;
    const float scale = 2.f;
    float cy = y + 4.f;

    if (m_Messages.empty()) {
        m_Renderer->DrawText("Console ready.", x + 6.f, cy, kLabelColor, scale);
        return;
    }

    // Show most recent messages that fit, scrolled to bottom
    int maxLines = static_cast<int>((h - 8.f) / lineH);
    int startIdx = static_cast<int>(m_Messages.size()) - maxLines;
    if (startIdx < 0) startIdx = 0;

    for (int i = startIdx; i < static_cast<int>(m_Messages.size()); ++i) {
        if (cy + lineH > y + h) break;
        m_Renderer->DrawText(m_Messages[static_cast<size_t>(i)],
                             x + 6.f, cy, kTextColor, scale);
        cy += lineH;
    }
}

} // namespace NF::Editor
