#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace UI {

// ──────────────────────────────────────────────────────────────
// Accessibility role for screen readers
// ──────────────────────────────────────────────────────────────

enum class AccessibilityRole {
    None,
    Button,
    Checkbox,
    ComboBox,
    Dialog,
    Heading,
    Image,
    Label,
    Link,
    ListItem,
    Menu,
    MenuItem,
    ProgressBar,
    Slider,
    Tab,
    TabPanel,
    TextBox,
    Tooltip,
    Tree,
    TreeItem
};

// ──────────────────────────────────────────────────────────────
// AccessibilityNode — metadata attached to a UI widget
// ──────────────────────────────────────────────────────────────

struct AccessibilityNode {
    std::string         id;
    std::string         label;         // human-readable name
    std::string         description;   // longer description
    AccessibilityRole   role  = AccessibilityRole::None;
    bool                focusable     = false;
    bool                focused       = false;
    bool                enabled       = true;
    bool                visible       = true;
    bool                expanded      = false; // for trees/accordions
    float               value         = 0.0f;  // slider/progress value [0,1]
    std::string         valueText;              // human-readable value string
    std::vector<std::string> children;          // child node IDs
};

// ──────────────────────────────────────────────────────────────
// AccessibilityManager — registry and focus management
// ──────────────────────────────────────────────────────────────

class AccessibilityManager {
public:
    // Node registration
    void RegisterNode(const AccessibilityNode& node);
    void UnregisterNode(const std::string& id);
    void UpdateNode(const AccessibilityNode& node);
    bool HasNode(const std::string& id) const;
    const AccessibilityNode* GetNode(const std::string& id) const;
    AccessibilityNode*       GetNode(const std::string& id);

    // Focus management
    void SetFocus(const std::string& id);
    void ClearFocus();
    const std::string& FocusedId() const { return m_focusedId; }
    bool FocusNext();
    bool FocusPrev();

    // Screen-reader announcement queue
    void Announce(const std::string& text, bool immediate = false);
    bool HasAnnouncements() const { return !m_announcements.empty(); }
    std::string PopAnnouncement();

    // Keyboard navigation shortcut list (key → action description)
    void RegisterShortcut(const std::string& key, const std::string& description);
    const std::unordered_map<std::string, std::string>& Shortcuts() const { return m_shortcuts; }

    // High-contrast mode
    void SetHighContrast(bool enabled);
    bool IsHighContrast() const { return m_highContrast; }

    // Font scale (1.0 = normal)
    void SetFontScale(float scale);
    float FontScale() const { return m_fontScale; }

    // Callbacks
    using FocusCallback   = std::function<void(const std::string& id)>;
    using AnnounceCallback = std::function<void(const std::string& text)>;
    void OnFocusChanged(FocusCallback cb)     { m_onFocus = std::move(cb); }
    void OnAnnounce(AnnounceCallback cb)      { m_onAnnounce = std::move(cb); }

    // Global singleton
    static AccessibilityManager& Instance();

private:
    std::unordered_map<std::string, AccessibilityNode> m_nodes;
    std::vector<std::string>                           m_focusOrder;
    std::string                                        m_focusedId;
    std::vector<std::string>                           m_announcements;
    std::unordered_map<std::string, std::string>       m_shortcuts;
    FocusCallback                                      m_onFocus;
    AnnounceCallback                                   m_onAnnounce;
    bool                                               m_highContrast = false;
    float                                              m_fontScale    = 1.0f;
};

} // namespace UI
