#include "AtlasUIPanels.h"
#include <algorithm>

namespace atlas::ui::atlas {

AtlasPanelManager::AtlasPanelManager() = default;
AtlasPanelManager::~AtlasPanelManager() = default;

// ---- Internal helpers ----

PanelState& AtlasPanelManager::GetOrCreate(const std::string& title,
                                           float w, float h) {
    auto it = m_panels.find(title);
    if (it != m_panels.end()) return it->second;

    PanelState ps;
    ps.title = title;
    ps.rect  = {50, 50, w, h};
    ps.open  = true;
    ps.zOrder = m_nextZOrder++;
    m_panels[title] = ps;
    return m_panels[title];
}

void AtlasPanelManager::BringToFront(PanelState& panel) {
    panel.zOrder = m_nextZOrder++;
}

// ---- Begin / End ----

bool AtlasPanelManager::BeginPanel(AtlasUIContext& ctx,
                                   const std::string& title,
                                   float defaultW, float defaultH) {
    auto& panel = GetOrCreate(title, defaultW, defaultH);
    m_currentPanel = title;

    if (!panel.open) return false;

    const auto& th = ctx.GetTheme();
    float titleH = static_cast<float>(th.panelTitleBar);

    // --- Title bar ---
    AtlasRect titleR{panel.rect.x, panel.rect.y, panel.rect.w, titleH};
    ctx.AddFilledRect(titleR, th.headerBackground);
    ctx.AddText({titleR.x + static_cast<float>(th.paddingSmall),
                 titleR.y + 2, titleR.w, titleH},
                title, th.textPrimary);

    // --- Drag handling ---
    bool titleHovered = titleR.Contains(ctx.Input().mouseX,
                                        ctx.Input().mouseY);
    if (titleHovered && ctx.Input().mouseClicked) {
        panel.dragging = true;
        panel.dragOffX = ctx.Input().mouseX - panel.rect.x;
        panel.dragOffY = ctx.Input().mouseY - panel.rect.y;
        BringToFront(panel);
    }
    if (panel.dragging) {
        if (ctx.Input().mouseDown) {
            panel.rect.x = ctx.Input().mouseX - panel.dragOffX;
            panel.rect.y = ctx.Input().mouseY - panel.dragOffY;
        } else {
            panel.dragging = false;
        }
    }

    // --- Panel body ---
    AtlasRect bodyR = panel.ContentRect(static_cast<int32_t>(titleH));
    ctx.AddFilledRect(bodyR, th.panelBackground);
    ctx.AddBorderRect(panel.rect, th.border, th.borderThickness);

    // Position layout cursor inside the panel content area.
    ctx.SetCursor(bodyR.x + static_cast<float>(th.paddingSmall),
                  bodyR.y + static_cast<float>(th.paddingSmall));

    return true;
}

void AtlasPanelManager::EndPanel(AtlasUIContext& /*ctx*/) {
    m_currentPanel.clear();
}

// ---- Open / close ----

void AtlasPanelManager::ClosePanel(const std::string& title) {
    auto it = m_panels.find(title);
    if (it != m_panels.end()) it->second.open = false;
}

void AtlasPanelManager::OpenPanel(const std::string& title) {
    auto it = m_panels.find(title);
    if (it != m_panels.end()) it->second.open = true;
}

void AtlasPanelManager::TogglePanel(const std::string& title) {
    auto it = m_panels.find(title);
    if (it != m_panels.end()) it->second.open = !it->second.open;
}

bool AtlasPanelManager::IsPanelOpen(const std::string& title) const {
    auto it = m_panels.find(title);
    return it != m_panels.end() && it->second.open;
}

void AtlasPanelManager::SetPanelPosition(const std::string& title,
                                         float x, float y) {
    auto it = m_panels.find(title);
    if (it != m_panels.end()) {
        it->second.rect.x = x;
        it->second.rect.y = y;
    }
}

const PanelState* AtlasPanelManager::GetPanel(const std::string& title) const {
    auto it = m_panels.find(title);
    return (it != m_panels.end()) ? &it->second : nullptr;
}

} // namespace atlas::ui::atlas
