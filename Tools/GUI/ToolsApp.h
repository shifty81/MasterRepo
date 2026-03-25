#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Tools/GUI/IToolPanel.h"
#include "UI/GUISystem/GUISystem.h"
#include "UI/Themes/UIStyle.h"

namespace Tools {

/**
 * ToolsApp — singleton host that owns all IToolPanel instances and drives
 * the update/draw loop using the project's custom UI framework.
 */
class ToolsApp {
public:
    static ToolsApp& Get();

    // Lifecycle
    void Init(int screenW, int screenH);
    void Shutdown();

    // Panel management
    void RegisterPanel(std::unique_ptr<IToolPanel> panel);
    void SetActivePanel(const std::string& name);
    const std::string& ActivePanelName() const;

    // Per-frame
    void Update(float dt);

    // Draw:
    //  1. Horizontal tab bar at the top (one button per panel)
    //  2. Active panel content area below via panel->OnDraw(style)
    void Draw();

    // Input routing — forwarded to GUISystem and then to the active panel
    void OnInput(float mouseX, float mouseY, int btn, bool btnPressed,
                 int key, bool keyPressed);

private:
    ToolsApp() = default;
    ToolsApp(const ToolsApp&) = delete;
    ToolsApp& operator=(const ToolsApp&) = delete;

    void RegisterDefaultPanels();

    std::vector<std::unique_ptr<IToolPanel>> m_panels;
    int                                       m_activeIndex{0};
    std::string                               m_activeName;
    int                                       m_screenW{1280};
    int                                       m_screenH{720};
    UI::UIStyle                               m_style;
    bool                                      m_initialised{false};
};

} // namespace Tools
