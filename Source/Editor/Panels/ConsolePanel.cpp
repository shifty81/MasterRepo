#include "Editor/Panels/ConsolePanel.h"
#include "UI/Rendering/UIRenderer.h"
#include <algorithm>

namespace NF::Editor {

void ConsolePanel::Update([[maybe_unused]] float dt) {
    // Handle mouse-wheel scrolling when the cursor is over this panel.
    // Actual bounds are not available here; scrolling is applied unconditionally
    // if the panel has focus (input is non-null and wheel moved).
    // Bounds-based hit-testing happens inside Draw().
}

void ConsolePanel::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;

    static constexpr uint32_t kTextColor  = 0x9CDCFEFF; // light cyan
    static constexpr uint32_t kLabelColor = 0x808080FF;

    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 18.f * dpi;
    const float scale = 2.f;
    const float padX  = 6.f * dpi;

    // Apply wheel scroll when cursor is inside the panel.
    if (m_Input && m_Input->wheelDelta != 0.f) {
        const float mx = m_Input->mouseX;
        const float my = m_Input->mouseY;
        if (mx >= x && mx < x + w && my >= y && my < y + h) {
            m_ScrollOffset += static_cast<int>(m_Input->wheelDelta);
        }
    }

    if (m_Messages.empty()) {
        m_Renderer->DrawText("Console ready.", x + padX, y + 4.f * dpi, kLabelColor, scale);
        return;
    }

    // How many lines fit in the panel?
    const int maxLines = static_cast<int>((h - 8.f * dpi) / lineH);

    // Clamp scroll so we don't go past the beginning or end.
    const int totalMessages = static_cast<int>(m_Messages.size());
    m_ScrollOffset = std::clamp(m_ScrollOffset, 0, std::max(0, totalMessages - maxLines));

    // startIdx: which message to show first (0 = oldest, scroll up to read older)
    // Default (offset 0) = show the most recent messages (scrolled to bottom).
    const int startIdx = std::max(0, totalMessages - maxLines - m_ScrollOffset);

    float cy = y + 4.f * dpi;
    for (int i = startIdx; i < totalMessages; ++i) {
        if (cy + lineH > y + h) break;
        m_Renderer->DrawText(m_Messages[static_cast<size_t>(i)],
                             x + padX, cy, kTextColor, scale);
        cy += lineH;
    }
}

} // namespace NF::Editor
