#pragma once
#include <string>
#include "Tools/GUI/IToolPanel.h"

namespace Tools {
class AssetBrowser;
}

namespace Tools {

/**
 * AssetBrowserPanel — path bar (current path + Up button), filter input,
 * entry list with type icons, Open/Import/Delete/Rename buttons, stats bar.
 * Uses Pimpl for browser state.
 */
class AssetBrowserPanel : public IToolPanel {
public:
    AssetBrowserPanel();
    ~AssetBrowserPanel() override;

    std::string PanelName() const override { return "Asset Browser"; }
    std::string PanelIcon() const override { return "[B]"; }

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnDraw(const UI::UIStyle& style) override;

    void SetBrowser(AssetBrowser* browser);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Tools
