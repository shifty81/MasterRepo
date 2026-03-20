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

} // namespace Editor
