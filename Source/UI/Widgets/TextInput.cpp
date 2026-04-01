#include "UI/Widgets/TextInput.h"

namespace NF {

void TextInput::SetText(const std::string& text) {
    m_Text = text;
    if (m_OnChanged) m_OnChanged(m_Text);
}

const std::string& TextInput::GetText() const noexcept {
    return m_Text;
}

void TextInput::SetPlaceholder(const std::string& placeholder) {
    m_Placeholder = placeholder;
}

void TextInput::SetOnChanged(std::function<void(std::string_view)> callback) {
    m_OnChanged = std::move(callback);
}

void TextInput::Update(float dt) {
    Widget::Update(dt);
    // Keyboard event routing from the input system would append/remove
    // characters here and fire m_OnChanged.
}

void TextInput::Draw() {
    // Concrete rendering deferred to the UIRenderer back-end.
    Widget::Draw();
}

} // namespace NF
