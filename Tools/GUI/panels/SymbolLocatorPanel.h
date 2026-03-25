#pragma once
#include <string>
#include "Tools/GUI/IToolPanel.h"

namespace Tools {
class SymbolLocator;
}

namespace Tools {

/**
 * SymbolLocatorPanel — search text input, Build Index button,
 * results list (name, kind, file, line), GoToDefinition info.
 */
class SymbolLocatorPanel : public IToolPanel {
public:
    SymbolLocatorPanel();
    ~SymbolLocatorPanel() override;

    std::string PanelName() const override { return "Symbol Locator"; }
    std::string PanelIcon() const override { return "[S]"; }

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnDraw(const UI::UIStyle& style) override;

    void SetLocator(SymbolLocator* locator);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Tools
