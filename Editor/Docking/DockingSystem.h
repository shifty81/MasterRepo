#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Editor {

// ──────────────────────────────────────────────────────────────
// DockingSystem — dockable panel layout manager
// ──────────────────────────────────────────────────────────────

enum class DockPosition { Left, Right, Top, Bottom, Center, Floating };

struct DockPanel {
    uint32_t    id        = 0;
    std::string title;
    DockPosition dockPos  = DockPosition::Center;
    float       width     = 300.f;
    float       height    = 200.f;
    float       x         = 0.f;
    float       y         = 0.f;
    bool        visible   = true;
    bool        closeable = true;
    bool        resizable = true;
};

class DockingSystem {
public:
    uint32_t AddPanel(const std::string& title, DockPosition pos = DockPosition::Center,
                      float w = 300.f, float h = 200.f);
    bool     RemovePanel(uint32_t id);
    DockPanel* GetPanel(uint32_t id);
    const std::vector<DockPanel>& Panels() const { return m_panels; }

    void ShowPanel(uint32_t id);
    void HidePanel(uint32_t id);
    void TogglePanel(uint32_t id);
    bool IsPanelVisible(uint32_t id) const;

    void DockPanel_(uint32_t id, DockPosition pos);
    void MovePanel(uint32_t id, float x, float y);
    void ResizePanel(uint32_t id, float w, float h);

    // Layout serialization
    std::string SerializeLayout() const;
    bool        DeserializeLayout(const std::string& json);

    void SetActivePanel(uint32_t id);
    uint32_t ActivePanel() const { return m_activeID; }

    using PanelClosedFn = std::function<void(uint32_t id)>;
    void SetPanelClosedCallback(PanelClosedFn fn);

private:
    uint32_t           m_nextID    = 1;
    uint32_t           m_activeID  = 0;
    std::vector<DockPanel> m_panels;
    PanelClosedFn      m_closedFn;
};

// ──────────────────────────────────────────────────────────────
// EditorModeRegistry — editor mode activation (migrated from Phase 3)
// ──────────────────────────────────────────────────────────────

struct EditorMode {
    uint32_t    id     = 0;
    std::string name;
    bool        active = false;
};

class EditorModeRegistry {
public:
    uint32_t RegisterMode(const std::string& name);
    bool     ActivateMode(const std::string& name);
    void     DeactivateAll();
    std::string ActiveModeName() const;
    const EditorMode* ActiveMode() const;
    const std::vector<EditorMode>& Modes() const { return m_modes; }

private:
    uint32_t             m_nextID = 1;
    std::vector<EditorMode> m_modes;
};

} // namespace Editor
