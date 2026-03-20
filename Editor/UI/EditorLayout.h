#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Editor {

// ─── Forward declarations ───────────────────────────────────────────────────

enum class DockSplit { None, Horizontal, Vertical, Tab };

class IPanel {
public:
    virtual ~IPanel() = default;
    virtual std::string Name() const     = 0;
    virtual bool  IsVisible() const      = 0;
    virtual void  SetVisible(bool v)     = 0;
};

// ─── DockNode ───────────────────────────────────────────────────────────────

struct DockNode {
    DockSplit                split      = DockSplit::None;
    float                    splitRatio = 0.5f;
    std::unique_ptr<DockNode> a;
    std::unique_ptr<DockNode> b;
    std::vector<IPanel*>     tabs;
    int                      activeTab  = 0;
};

// ─── MenuBar ────────────────────────────────────────────────────────────────

struct MenuBar {
    std::function<void()> onSaveLayout;
    std::function<void()> onLoadLayout;
    std::function<void()> onNewScene;
    std::function<void()> onOpenScene;
    std::function<void()> onSaveScene;
    void Build() {} // placeholder
};

// ─── EditorLayout ───────────────────────────────────────────────────────────

class EditorLayout {
public:
    EditorLayout() = default;

    void RegisterPanel(IPanel* panel);
    const std::vector<IPanel*>& Panels() const;

    DockNode&  Root();
    MenuBar&   GetMenuBar();

    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);

private:
    std::vector<IPanel*> m_panels;
    DockNode             m_root;
    ::Editor::MenuBar    m_menuBar;
};

} // namespace Editor
