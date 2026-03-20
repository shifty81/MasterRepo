#include "UI/Accessibility/Accessibility.h"
#include <algorithm>

namespace UI {

// ── node management ────────────────────────────────────────────

void AccessibilityManager::RegisterNode(const AccessibilityNode& node) {
    m_nodes[node.id] = node;
    if (std::find(m_focusOrder.begin(), m_focusOrder.end(), node.id) == m_focusOrder.end())
        if (node.focusable) m_focusOrder.push_back(node.id);
}

void AccessibilityManager::UnregisterNode(const std::string& id) {
    m_nodes.erase(id);
    m_focusOrder.erase(std::remove(m_focusOrder.begin(), m_focusOrder.end(), id),
                       m_focusOrder.end());
    if (m_focusedId == id) ClearFocus();
}

void AccessibilityManager::UpdateNode(const AccessibilityNode& node) {
    m_nodes[node.id] = node;
}

bool AccessibilityManager::HasNode(const std::string& id) const {
    return m_nodes.count(id) > 0;
}

const AccessibilityNode* AccessibilityManager::GetNode(const std::string& id) const {
    auto it = m_nodes.find(id);
    return (it != m_nodes.end()) ? &it->second : nullptr;
}

AccessibilityNode* AccessibilityManager::GetNode(const std::string& id) {
    auto it = m_nodes.find(id);
    return (it != m_nodes.end()) ? &it->second : nullptr;
}

// ── focus ──────────────────────────────────────────────────────

void AccessibilityManager::SetFocus(const std::string& id) {
    if (!m_focusedId.empty())
        if (auto* n = GetNode(m_focusedId)) n->focused = false;

    m_focusedId = id;
    if (auto* n = GetNode(id)) {
        n->focused = true;
        Announce(n->label.empty() ? id : n->label);
    }
    if (m_onFocus) m_onFocus(id);
}

void AccessibilityManager::ClearFocus() {
    if (!m_focusedId.empty())
        if (auto* n = GetNode(m_focusedId)) n->focused = false;
    m_focusedId.clear();
}

bool AccessibilityManager::FocusNext() {
    if (m_focusOrder.empty()) return false;
    auto it = std::find(m_focusOrder.begin(), m_focusOrder.end(), m_focusedId);
    if (it == m_focusOrder.end() || ++it == m_focusOrder.end())
        it = m_focusOrder.begin();
    SetFocus(*it);
    return true;
}

bool AccessibilityManager::FocusPrev() {
    if (m_focusOrder.empty()) return false;
    auto it = std::find(m_focusOrder.begin(), m_focusOrder.end(), m_focusedId);
    if (it == m_focusOrder.begin()) it = m_focusOrder.end();
    SetFocus(*--it);
    return true;
}

// ── announcements ─────────────────────────────────────────────

void AccessibilityManager::Announce(const std::string& text, bool immediate) {
    if (immediate) {
        if (m_onAnnounce) m_onAnnounce(text);
        return;
    }
    m_announcements.push_back(text);
}

std::string AccessibilityManager::PopAnnouncement() {
    if (m_announcements.empty()) return {};
    std::string msg = m_announcements.front();
    m_announcements.erase(m_announcements.begin());
    if (m_onAnnounce) m_onAnnounce(msg);
    return msg;
}

// ── shortcuts / display settings ──────────────────────────────

void AccessibilityManager::RegisterShortcut(const std::string& key,
                                             const std::string& description) {
    m_shortcuts[key] = description;
}

void AccessibilityManager::SetHighContrast(bool enabled) { m_highContrast = enabled; }
void AccessibilityManager::SetFontScale(float scale)     { m_fontScale = scale; }

// ── singleton ─────────────────────────────────────────────────

AccessibilityManager& AccessibilityManager::Instance() {
    static AccessibilityManager instance;
    return instance;
}

} // namespace UI
