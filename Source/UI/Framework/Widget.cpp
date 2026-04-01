#include "UI/Framework/Widget.h"

namespace NF {

void Widget::SetRect(const Rect& rect) {
    m_Rect = rect;
}

const Rect& Widget::GetRect() const noexcept {
    return m_Rect;
}

void Widget::Update(float dt) {
    for (auto& child : m_Children)
        child->Update(dt);
}

void Widget::Draw() {
    for (auto& child : m_Children)
        child->Draw();
}

void Widget::AddChild(std::shared_ptr<Widget> child) {
    m_Children.push_back(std::move(child));
}

const std::vector<std::shared_ptr<Widget>>& Widget::GetChildren() const noexcept {
    return m_Children;
}

} // namespace NF
