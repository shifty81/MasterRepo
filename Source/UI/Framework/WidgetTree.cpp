#include "UI/Framework/WidgetTree.h"

namespace NF {

void WidgetTree::SetRoot(std::shared_ptr<Widget> root) {
    m_Root = std::move(root);
}

void WidgetTree::Update(float dt) {
    if (m_Root) m_Root->Update(dt);
}

void WidgetTree::Draw() {
    if (m_Root) m_Root->Draw();
}

Widget* WidgetTree::HitTest(const Vector2& pos) const {
    if (!m_Root) return nullptr;
    return HitTestNode(m_Root.get(), pos);
}

Widget* WidgetTree::HitTestNode(Widget* widget, const Vector2& pos) const {
    if (!widget) return nullptr;

    const Rect& r = widget->GetRect();
    if (pos.X < r.X || pos.X > r.X + r.Width ||
        pos.Y < r.Y || pos.Y > r.Y + r.Height)
        return nullptr;

    // Depth-first: check children last-to-first (topmost drawn last).
    const auto& children = widget->GetChildren();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        if (Widget* hit = HitTestNode(it->get(), pos))
            return hit;
    }
    return widget;
}

} // namespace NF
