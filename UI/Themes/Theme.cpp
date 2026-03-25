#include "UI/Themes/Theme.h"

namespace UI {

// ──────────────────────────────────────────────────────────────
// Theme
// ──────────────────────────────────────────────────────────────

uint32_t Theme::GetColor(ThemeColor c) const {
    auto it = colors.find(c);
    return (it != colors.end()) ? it->second : 0u;
}

void Theme::SetColor(ThemeColor c, uint32_t rgba) {
    colors[c] = rgba;
}

ThemeFont Theme::GetFont(const std::string& role) const {
    auto it = fonts.find(role);
    if (it != fonts.end()) return it->second;
    // Fall back to default role
    auto def = fonts.find("default");
    return (def != fonts.end()) ? def->second : ThemeFont{};
}

void Theme::SetFont(const std::string& role, ThemeFont font) {
    fonts[role] = std::move(font);
}

void Theme::ApplyTo(WidgetStyle& style, WidgetType type) const {
    style.fgColor      = GetColor(ThemeColor::Text);
    style.borderColor  = GetColor(ThemeColor::Border);
    style.borderWidth  = borderWidth;
    style.padding      = padding;
    style.cornerRadius = cornerRadius;

    const ThemeFont& df = GetFont("default");
    style.fontSize  = df.size;
    style.fontName  = df.name;

    switch (type) {
        case WidgetType::Button:
            style.bgColor = GetColor(ThemeColor::ButtonBg);
            break;
        case WidgetType::TextInput:
            style.bgColor     = GetColor(ThemeColor::InputBg);
            style.borderColor = GetColor(ThemeColor::InputBorder);
            break;
        case WidgetType::Panel:
        case WidgetType::ScrollArea:
            style.bgColor = GetColor(ThemeColor::PanelBg);
            break;
        case WidgetType::Label:
        case WidgetType::Separator:
            style.bgColor = 0u; // transparent
            break;
        default:
            style.bgColor = GetColor(ThemeColor::Surface);
            break;
    }
}

} // namespace UI
