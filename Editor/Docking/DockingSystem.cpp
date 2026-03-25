#include "Editor/Docking/DockingSystem.h"
#include <algorithm>
#include <sstream>

namespace Editor {

// ──────────────────────────────────────────────────────────────
// DockingSystem
// ──────────────────────────────────────────────────────────────

uint32_t DockingSystem::AddPanel(const std::string& title, DockPosition pos,
                                  float w, float h) {
    DockPanel p;
    p.id      = m_nextID++;
    p.title   = title;
    p.dockPos = pos;
    p.width   = w;
    p.height  = h;
    m_panels.push_back(std::move(p));
    return m_panels.back().id;
}

bool DockingSystem::RemovePanel(uint32_t id) {
    auto it = std::find_if(m_panels.begin(), m_panels.end(),
        [id](const DockPanel& p){ return p.id == id; });
    if (it == m_panels.end()) return false;
    if (m_closedFn && it->closeable) m_closedFn(id);
    m_panels.erase(it);
    return true;
}

DockPanel* DockingSystem::GetPanel(uint32_t id) {
    for (auto& p : m_panels) if (p.id == id) return &p;
    return nullptr;
}

void DockingSystem::ShowPanel(uint32_t id) {
    if (auto* p = GetPanel(id)) p->visible = true;
}

void DockingSystem::HidePanel(uint32_t id) {
    if (auto* p = GetPanel(id)) p->visible = false;
}

void DockingSystem::TogglePanel(uint32_t id) {
    if (auto* p = GetPanel(id)) p->visible = !p->visible;
}

bool DockingSystem::IsPanelVisible(uint32_t id) const {
    for (const auto& p : m_panels) if (p.id == id) return p.visible;
    return false;
}

void DockingSystem::DockPanel_(uint32_t id, DockPosition pos) {
    if (auto* p = GetPanel(id)) p->dockPos = pos;
}

void DockingSystem::MovePanel(uint32_t id, float x, float y) {
    if (auto* p = GetPanel(id)) { p->x = x; p->y = y; p->dockPos = DockPosition::Floating; }
}

void DockingSystem::ResizePanel(uint32_t id, float w, float h) {
    if (auto* p = GetPanel(id)) { p->width = w; p->height = h; }
}

void DockingSystem::SetActivePanel(uint32_t id) { m_activeID = id; }
void DockingSystem::SetPanelClosedCallback(PanelClosedFn fn) { m_closedFn = std::move(fn); }

std::string DockingSystem::SerializeLayout() const {
    std::ostringstream os;
    os << "[";
    for (size_t i = 0; i < m_panels.size(); ++i) {
        const auto& p = m_panels[i];
        if (i) os << ",";
        os << "{\"id\":" << p.id
           << ",\"title\":\"" << p.title << "\""
           << ",\"visible\":" << (p.visible ? "true" : "false")
           << ",\"dock\":" << (int)p.dockPos
           << ",\"x\":" << p.x << ",\"y\":" << p.y
           << ",\"w\":" << p.width << ",\"h\":" << p.height << "}";
    }
    os << "]";
    return os.str();
}

bool DockingSystem::DeserializeLayout(const std::string& /*json*/) {
    // Stub: full JSON parse would go here
    return true;
}

// ──────────────────────────────────────────────────────────────
// EditorModeRegistry
// ──────────────────────────────────────────────────────────────

uint32_t EditorModeRegistry::RegisterMode(const std::string& name) {
    for (const auto& m : m_modes) if (m.name == name) return m.id;
    EditorMode em; em.id = m_nextID++; em.name = name;
    m_modes.push_back(std::move(em));
    return m_modes.back().id;
}

bool EditorModeRegistry::ActivateMode(const std::string& name) {
    for (auto& m : m_modes) m.active = false;
    for (auto& m : m_modes) {
        if (m.name == name) { m.active = true; return true; }
    }
    return false;
}

void EditorModeRegistry::DeactivateAll() {
    for (auto& m : m_modes) m.active = false;
}

std::string EditorModeRegistry::ActiveModeName() const {
    for (const auto& m : m_modes) if (m.active) return m.name;
    return "";
}

const EditorMode* EditorModeRegistry::ActiveMode() const {
    for (const auto& m : m_modes) if (m.active) return &m;
    return nullptr;
}

// ── PL-01: Mouse drag/resize splitter bars ────────────────────────────────

bool DockingSystem::OnMouseButton(int btn, bool pressed, double x, double y) {
    if (btn != 0) return false;

    if (pressed) {
        // Check if mouse is near an edge of a floating panel to initiate resize,
        // or inside a panel's title bar to initiate a move/drag.
        for (auto& p : m_panels) {
            if (!p.visible) continue;

            float fx = (float)x, fy = (float)y;
            bool inPanel = (fx >= p.x && fx < p.x + p.width &&
                            fy >= p.y && fy < p.y + p.height);
            if (!inPanel) continue;

            bool nearRight  = (fx >= p.x + p.width  - kEdgeSlop);
            bool nearBottom = (fy >= p.y + p.height - kEdgeSlop);

            m_dragStartX  = x;
            m_dragStartY  = y;
            m_dragOriginX = p.x;
            m_dragOriginY = p.y;
            m_dragOriginW = p.width;
            m_dragOriginH = p.height;
            m_dragPanelID = p.id;

            if ((nearRight || nearBottom) && p.resizable) {
                m_resizing     = true;
                m_dragging     = false;
                m_resizeRight  = nearRight;
                m_resizeBottom = nearBottom;
            } else {
                // Title bar drag (top 20px of panel)
                if (fy < p.y + 20.f) {
                    m_dragging = true;
                    m_resizing = false;
                    p.dockPos  = DockPosition::Floating;
                }
            }
            SetActivePanel(p.id);
            return m_dragging || m_resizing;
        }
    } else {
        m_dragging = m_resizing = false;
        m_dragPanelID = 0;
    }
    return false;
}

bool DockingSystem::OnMouseMove(double x, double y) {
    if (!m_dragging && !m_resizing) return false;
    DockPanel* p = GetPanel(m_dragPanelID);
    if (!p) { m_dragging = m_resizing = false; return false; }

    float dx = (float)(x - m_dragStartX);
    float dy = (float)(y - m_dragStartY);

    if (m_dragging) {
        p->x = m_dragOriginX + dx;
        p->y = m_dragOriginY + dy;
    } else if (m_resizing) {
        if (m_resizeRight)  p->width  = std::max(80.f, m_dragOriginW + dx);
        if (m_resizeBottom) p->height = std::max(40.f, m_dragOriginH + dy);
    }
    return true;
}

} // namespace Editor
