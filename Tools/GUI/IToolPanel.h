#pragma once
#include <string>
#include "UI/Themes/UIStyle.h"

namespace Tools {

/**
 * IToolPanel — abstract base for every panel hosted inside ToolsApp.
 *
 * Concrete panels override PanelName(), OnDraw(), and any lifecycle/input
 * methods they care about.  The host calls the lifecycle methods in order:
 *   OnAttach → { OnUpdate / OnDraw }* → OnDetach
 */
class IToolPanel {
public:
    virtual ~IToolPanel() = default;

    virtual std::string PanelName() const = 0;
    virtual std::string PanelIcon() const { return ""; }  // e.g. emoji or short ASCII

    virtual void OnAttach()  {}
    virtual void OnDetach()  {}
    virtual void OnUpdate(float /*dt*/) {}

    // Draws panel content using the custom UI primitives.
    // The style carries colours / spacing for layout hints.
    virtual void OnDraw(const UI::UIStyle& style) = 0;

    virtual bool IsVisible() const      { return m_visible; }
    virtual void SetVisible(bool v)     { m_visible = v; }

    virtual void OnKeyEvent(int /*key*/, bool /*pressed*/) {}
    virtual void OnMouseEvent(float /*x*/, float /*y*/, int /*btn*/, bool /*pressed*/) {}

protected:
    bool m_visible{true};
};

} // namespace Tools
