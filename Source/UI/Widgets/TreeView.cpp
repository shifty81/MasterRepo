#include "UI/Widgets/TreeView.h"

namespace NF {

void TreeView::SetItems(std::vector<TreeItem> items) {
    m_Items    = std::move(items);
    m_Selected = nullptr;
}

const TreeItem* TreeView::GetSelectedItem() const noexcept {
    return m_Selected;
}

void TreeView::Update(float dt) {
    Widget::Update(dt);
}

void TreeView::Draw() {
    // Concrete rendering deferred to the UIRenderer back-end.
    Widget::Draw();
}

} // namespace NF
