#include "Tools/GUI/ToolsApp.h"

#include <algorithm>
#include <string>

#include "UI/Widgets/Widget.h"

// Panel headers — all panels are registered here
#include "Tools/GUI/panels/GPUProfilerPanel.h"
#include "Tools/GUI/panels/PerfProfilerPanel.h"
#include "Tools/GUI/panels/MemoryProfilerPanel.h"
#include "Tools/GUI/panels/DependencyPanel.h"
#include "Tools/GUI/panels/SymbolLocatorPanel.h"
#include "Tools/GUI/panels/GitHistoryPanel.h"
#include "Tools/GUI/panels/AtlasPackerPanel.h"
#include "Tools/GUI/panels/AssetBrowserPanel.h"

namespace Tools {

// ── Singleton ─────────────────────────────────────────────────────────────────

ToolsApp& ToolsApp::Get() {
    static ToolsApp instance;
    return instance;
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void ToolsApp::Init(int screenW, int screenH) {
    if (m_initialised) return;
    m_screenW      = screenW;
    m_screenH      = screenH;
    m_style        = UI::UIStyle::Dark();
    m_initialised  = true;

    RegisterDefaultPanels();

    // Attach all panels; activate the first one
    for (auto& p : m_panels) p->OnAttach();
    if (!m_panels.empty()) {
        m_activeIndex = 0;
        m_activeName  = m_panels[0]->PanelName();
    }
}

void ToolsApp::Shutdown() {
    for (auto& p : m_panels) p->OnDetach();
    m_panels.clear();
    m_initialised = false;
}

// ── Panel management ──────────────────────────────────────────────────────────

void ToolsApp::RegisterPanel(std::unique_ptr<IToolPanel> panel) {
    if (panel) m_panels.push_back(std::move(panel));
}

void ToolsApp::SetActivePanel(const std::string& name) {
    for (int i = 0; i < static_cast<int>(m_panels.size()); ++i) {
        if (m_panels[i]->PanelName() == name) {
            m_activeIndex = i;
            m_activeName  = name;
            return;
        }
    }
}

const std::string& ToolsApp::ActivePanelName() const { return m_activeName; }

// ── Per-frame ─────────────────────────────────────────────────────────────────

void ToolsApp::Update(float dt) {
    for (auto& p : m_panels)
        if (p->IsVisible()) p->OnUpdate(dt);
}

void ToolsApp::Draw() {
    if (m_panels.empty()) return;

    // ── Tab bar ───────────────────────────────────────────────────────────────
    // One Button per panel across the top of the window.
    // [RENDER: fill header bar with style.headerBackground, height ~30px]
    const float tabH   = 30.f;
    const float tabW   = static_cast<float>(m_screenW) / static_cast<float>(m_panels.size());
    float       tabX   = 0.f;

    for (int i = 0; i < static_cast<int>(m_panels.size()); ++i) {
        IToolPanel& panel = *m_panels[i];

        UI::Button tab(panel.PanelIcon() + " " + panel.PanelName());
        tab.SetBounds({tabX, 0.f, tabW - 2.f, tabH});

        // Highlight the active tab with accent colour
        if (i == m_activeIndex) {
            UI::WidgetStyle ts = tab.GetStyle();
            ts.bgColor = 0x007ACCFF; // accent blue
            tab.SetStyle(ts);
        }

        tab.SetOnClick([this, i]() {
            m_activeIndex = i;
            m_activeName  = m_panels[i]->PanelName();
        });
        // [RENDER: draw tab button at (tabX, 0, tabW-2, tabH)]
        tab.OnEvent(UI::WidgetEvent::None, tabX, 0.f);
        tabX += tabW;
    }

    // ── Content area ──────────────────────────────────────────────────────────
    // [RENDER: content region starts at y=tabH and fills remaining screen]
    if (m_activeIndex < static_cast<int>(m_panels.size())) {
        IToolPanel& active = *m_panels[m_activeIndex];
        if (active.IsVisible()) active.OnDraw(m_style);
    }
}

// ── Input routing ─────────────────────────────────────────────────────────────

void ToolsApp::OnInput(float mouseX, float mouseY, int /*btn*/, bool btnPressed,
                       int key, bool keyPressed) {
    // Forward to active panel
    if (m_activeIndex < static_cast<int>(m_panels.size())) {
        m_panels[m_activeIndex]->OnMouseEvent(mouseX, mouseY, 0, btnPressed);
        if (keyPressed) m_panels[m_activeIndex]->OnKeyEvent(key, keyPressed);
    }
}

// ── Default panel registration ────────────────────────────────────────────────

void ToolsApp::RegisterDefaultPanels() {
    RegisterPanel(std::make_unique<GPUProfilerPanel>());
    RegisterPanel(std::make_unique<PerfProfilerPanel>());
    RegisterPanel(std::make_unique<MemoryProfilerPanel>());
    RegisterPanel(std::make_unique<DependencyPanel>());
    RegisterPanel(std::make_unique<SymbolLocatorPanel>());
    RegisterPanel(std::make_unique<GitHistoryPanel>());
    RegisterPanel(std::make_unique<AtlasPackerPanel>());
    RegisterPanel(std::make_unique<AssetBrowserPanel>());
}

} // namespace Tools
