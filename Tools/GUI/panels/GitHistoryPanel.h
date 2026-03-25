#pragma once
#include <string>
#include "Tools/GUI/IToolPanel.h"

namespace Tools {
class GitHistory;
}

namespace Tools {

/**
 * GitHistoryPanel — repo path input, Load button, commit table,
 * author/keyword filters, diff view for the selected commit.
 * Uses Pimpl to hide GitHistory state.
 */
class GitHistoryPanel : public IToolPanel {
public:
    GitHistoryPanel();
    ~GitHistoryPanel() override;

    std::string PanelName() const override { return "Git History"; }
    std::string PanelIcon() const override { return "[G]"; }

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnDraw(const UI::UIStyle& style) override;

    void SetHistory(GitHistory* history);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Tools
