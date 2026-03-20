#include "Editor/Overlay/OverlaySystem.h"
#include <algorithm>

namespace Editor {

uint32_t OverlayPanel::PostNotification(const std::string& msg, NotificationLevel level, uint32_t duration_ms) {
    Notification n;
    n.id          = m_nextId++;
    n.message     = msg;
    n.level       = level;
    n.timestamp   = m_elapsed;
    n.duration_ms = duration_ms;
    n.dismissed   = false;
    m_notifications.push_back(n);
    return n.id;
}

void OverlayPanel::DismissNotification(uint32_t id) {
    for (auto& n : m_notifications)
        if (n.id == id) { n.dismissed = true; break; }
}

void OverlayPanel::Tick(uint32_t delta_ms) {
    m_elapsed += delta_ms;
    for (auto& n : m_notifications) {
        if (!n.dismissed && m_elapsed >= n.timestamp + n.duration_ms)
            n.dismissed = true;
    }
    m_notifications.erase(
        std::remove_if(m_notifications.begin(), m_notifications.end(),
                       [](const Notification& n) { return n.dismissed; }),
        m_notifications.end());
}

uint32_t OverlayPanel::AddDebugEntry(const std::string& category, const std::string& key,
                                      const std::string& value, uint32_t color) {
    DebugOverlay d;
    d.id       = m_nextId++;
    d.category = category;
    d.key      = key;
    d.value    = value;
    d.color    = color;
    d.visible  = true;
    m_debugEntries.push_back(d);
    return d.id;
}

void OverlayPanel::UpdateDebugEntry(uint32_t id, const std::string& value) {
    for (auto& d : m_debugEntries)
        if (d.id == id) { d.value = value; break; }
}

void OverlayPanel::RemoveDebugEntry(uint32_t id) {
    m_debugEntries.erase(
        std::remove_if(m_debugEntries.begin(), m_debugEntries.end(),
                       [id](const DebugOverlay& d) { return d.id == id; }),
        m_debugEntries.end());
}

uint32_t OverlayPanel::RegisterToolOverlay(const std::string& toolName, const std::string& helpText,
                                             const std::string& shortcut) {
    ToolOverlay t;
    t.id       = m_nextId++;
    t.toolName = toolName;
    t.helpText = helpText;
    t.shortcut = shortcut;
    t.visible  = true;
    m_toolOverlays.push_back(t);
    return t.id;
}

void OverlayPanel::SetToolOverlayVisible(uint32_t id, bool visible) {
    for (auto& t : m_toolOverlays)
        if (t.id == id) { t.visible = visible; break; }
}

void OverlayPanel::ClearAll() {
    m_notifications.clear();
    m_debugEntries.clear();
    m_toolOverlays.clear();
}

} // namespace Editor
