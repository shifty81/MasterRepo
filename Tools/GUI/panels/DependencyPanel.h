#pragma once
#include <string>
#include "Tools/GUI/IToolPanel.h"

namespace Tools {
class DependencyAnalyzer;
}

namespace Tools {

/**
 * DependencyPanel — front-end for DependencyAnalyzer: search root input,
 * Analyse button, top fanout/fanin table, cycle list, file count.
 */
class DependencyPanel : public IToolPanel {
public:
    DependencyPanel();
    ~DependencyPanel() override;

    std::string PanelName() const override { return "Dependencies"; }
    std::string PanelIcon() const override { return "[D]"; }

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnDraw(const UI::UIStyle& style) override;

    void SetAnalyzer(DependencyAnalyzer* analyzer);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Tools
