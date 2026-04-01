#pragma once
#include "UI/Framework/Widget.h"
#include <functional>
#include <string>
#include <string_view>

namespace NF {

/// @brief Single-line text input widget.
class TextInput : public Widget {
public:
    /// @brief Set the current text value.
    void SetText(const std::string& text);

    /// @brief Return the current text value.
    [[nodiscard]] const std::string& GetText() const noexcept;

    /// @brief Set the placeholder text shown when the field is empty.
    void SetPlaceholder(const std::string& placeholder);

    /// @brief Register a callback invoked whenever the text changes.
    /// @param callback Called with the new text value.
    void SetOnChanged(std::function<void(std::string_view)> callback);

    /// @brief Advance input state.
    /// @param dt Delta time in seconds.
    void Update(float dt) override;

    /// @brief Render the text field.
    void Draw() override;

private:
    std::string                          m_Text;
    std::string                          m_Placeholder;
    std::function<void(std::string_view)> m_OnChanged;
};

} // namespace NF
