#pragma once
#include <string>
#include "Tools/GUI/IToolPanel.h"

namespace Tools {
class TextureAtlasPacker;
}

namespace Tools {

/**
 * AtlasPackerPanel — config inputs (page size, padding, algo selector),
 * texture list, Add/Remove/Clear buttons, Pack button, result stats.
 * Uses Pimpl for atlas state.
 */
class AtlasPackerPanel : public IToolPanel {
public:
    AtlasPackerPanel();
    ~AtlasPackerPanel() override;

    std::string PanelName() const override { return "Atlas Packer"; }
    std::string PanelIcon() const override { return "[A]"; }

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnDraw(const UI::UIStyle& style) override;

    void SetPacker(TextureAtlasPacker* packer);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Tools
