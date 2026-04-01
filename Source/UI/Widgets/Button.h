#pragma once
#include "UI/Framework/Widget.h"
#include <functional>
#include <string>

namespace NF {

/// @brief A clickable button widget with a text label.
class Button : public Widget {
public:
    /// @brief Set the visible label text.
    void SetLabel(const std::string& label);

    /// @brief Register a callback to invoke when the button is clicked.
    /// @param callback Zero-argument callable.
    void SetOnClick(std::function<void()> callback);

    /// @brief Advance button state; fires the click callback on simulated input.
    /// @param dt Delta time in seconds.
    void Update(float dt) override;

    /// @brief Render the button background and label.
    void Draw() override;

private:
    std::string           m_Label;
    std::function<void()> m_OnClick;
};

} // namespace NF
