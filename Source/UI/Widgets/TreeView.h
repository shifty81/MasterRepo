#pragma once
#include "UI/Framework/Widget.h"
#include <string>
#include <vector>

namespace NF {

/// @brief A node in a hierarchical tree view.
struct TreeItem {
    std::string           Label;
    bool                  Expanded{false};
    std::vector<TreeItem> Children;
};

/// @brief Collapsible tree-view widget.
class TreeView : public Widget {
public:
    /// @brief Replace the item collection.
    /// @param items New top-level items to display.
    void SetItems(std::vector<TreeItem> items);

    /// @brief Return a pointer to the currently selected item, or nullptr.
    [[nodiscard]] const TreeItem* GetSelectedItem() const noexcept;

    /// @brief Advance tree-view state.
    /// @param dt Delta time in seconds.
    void Update(float dt) override;

    /// @brief Render the visible tree nodes.
    void Draw() override;

private:
    std::vector<TreeItem> m_Items;
    const TreeItem*       m_Selected{nullptr};
};

} // namespace NF
