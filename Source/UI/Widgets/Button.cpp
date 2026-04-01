#include "UI/Widgets/Button.h"

namespace NF {

void Button::SetLabel(const std::string& label) {
    m_Label = label;
}

void Button::SetOnClick(std::function<void()> callback) {
    m_OnClick = std::move(callback);
}

void Button::Update(float dt) {
    Widget::Update(dt);
    // Click handling is triggered externally (e.g. via input system hit-testing).
    // The callback is exposed here for programmatic invocation.
}

void Button::Draw() {
    // Concrete rendering deferred to the UIRenderer back-end.
    Widget::Draw();
}

} // namespace NF
