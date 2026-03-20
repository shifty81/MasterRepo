#include "UI/Themes/Theme.h"
#include <stdexcept>

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

// ──────────────────────────────────────────────────────────────
// ThemeManager
// ──────────────────────────────────────────────────────────────

ThemeManager::ThemeManager() {
    Theme dark  = MakeDarkTheme();
    Theme light = MakeLightTheme();
    m_activeName = dark.name;
    m_themes[dark.name]  = std::move(dark);
    m_themes[light.name] = std::move(light);
}

void ThemeManager::RegisterTheme(Theme theme) {
    if (m_activeName.empty()) m_activeName = theme.name;
    m_themes[theme.name] = std::move(theme);
}

bool ThemeManager::SetActiveTheme(const std::string& name) {
    if (m_themes.find(name) == m_themes.end()) return false;
    m_activeName = name;
    return true;
}

const Theme& ThemeManager::GetActiveTheme() const {
    auto it = m_themes.find(m_activeName);
    if (it == m_themes.end())
        throw std::runtime_error("ThemeManager: no active theme");
    return it->second;
}

const Theme* ThemeManager::GetTheme(const std::string& name) const {
    auto it = m_themes.find(name);
    return (it != m_themes.end()) ? &it->second : nullptr;
}

std::vector<std::string> ThemeManager::ThemeNames() const {
    std::vector<std::string> out;
    out.reserve(m_themes.size());
    for (const auto& [k, _] : m_themes) out.push_back(k);
    return out;
}

// ──────────────────────────────────────────────────────────────
// Factory — Dark theme
// ──────────────────────────────────────────────────────────────

/*static*/ Theme ThemeManager::MakeDarkTheme() {
    Theme t;
    t.name = "Dark";

    t.SetColor(ThemeColor::Background,    0x1A1A1AFF);
    t.SetColor(ThemeColor::Surface,       0x252525FF);
    t.SetColor(ThemeColor::Primary,       0x4A90D9FF);
    t.SetColor(ThemeColor::Secondary,     0x5B5EA6FF);
    t.SetColor(ThemeColor::Accent,        0xF5A623FF);
    t.SetColor(ThemeColor::Text,          0xE8E8E8FF);
    t.SetColor(ThemeColor::TextMuted,     0x888888FF);
    t.SetColor(ThemeColor::Border,        0x3A3A3AFF);
    t.SetColor(ThemeColor::Error,         0xE05252FF);
    t.SetColor(ThemeColor::Warning,       0xF5C242FF);
    t.SetColor(ThemeColor::Success,       0x5CB85CFF);
    t.SetColor(ThemeColor::ButtonBg,      0x3C3C3CFF);
    t.SetColor(ThemeColor::ButtonHover,   0x4A90D9FF);
    t.SetColor(ThemeColor::ButtonPressed, 0x2A6099FF);
    t.SetColor(ThemeColor::InputBg,       0x1E1E1EFF);
    t.SetColor(ThemeColor::InputBorder,   0x555555FF);
    t.SetColor(ThemeColor::PanelBg,       0x2B2B2BFF);
    t.SetColor(ThemeColor::OverlayBg,     0x000000CC);
    t.SetColor(ThemeColor::ScrollBar,     0x404040FF);

    t.SetFont("default", {"default", 14.f, false, false});
    t.SetFont("heading",  {"default", 18.f, true,  false});
    t.SetFont("code",     {"mono",    13.f, false, false});
    t.SetFont("small",    {"default", 11.f, false, false});

    t.cornerRadius = 4.f;
    t.borderWidth  = 1.f;
    t.padding      = 6.f;
    t.spacing      = 4.f;
    return t;
}

// ──────────────────────────────────────────────────────────────
// Factory — Light theme
// ──────────────────────────────────────────────────────────────

/*static*/ Theme ThemeManager::MakeLightTheme() {
    Theme t;
    t.name = "Light";

    t.SetColor(ThemeColor::Background,    0xF0F0F0FF);
    t.SetColor(ThemeColor::Surface,       0xFFFFFFFF);
    t.SetColor(ThemeColor::Primary,       0x2678C8FF);
    t.SetColor(ThemeColor::Secondary,     0x5B5EA6FF);
    t.SetColor(ThemeColor::Accent,        0xE07B00FF);
    t.SetColor(ThemeColor::Text,          0x1A1A1AFF);
    t.SetColor(ThemeColor::TextMuted,     0x666666FF);
    t.SetColor(ThemeColor::Border,        0xCCCCCCFF);
    t.SetColor(ThemeColor::Error,         0xCC0000FF);
    t.SetColor(ThemeColor::Warning,       0xCC8800FF);
    t.SetColor(ThemeColor::Success,       0x2E7D32FF);
    t.SetColor(ThemeColor::ButtonBg,      0xE0E0E0FF);
    t.SetColor(ThemeColor::ButtonHover,   0x2678C8FF);
    t.SetColor(ThemeColor::ButtonPressed, 0x1A5599FF);
    t.SetColor(ThemeColor::InputBg,       0xFFFFFFFF);
    t.SetColor(ThemeColor::InputBorder,   0xAAAAAAFF);
    t.SetColor(ThemeColor::PanelBg,       0xF8F8F8FF);
    t.SetColor(ThemeColor::OverlayBg,     0x00000066);
    t.SetColor(ThemeColor::ScrollBar,     0xBBBBBBFF);

    t.SetFont("default", {"default", 14.f, false, false});
    t.SetFont("heading",  {"default", 18.f, true,  false});
    t.SetFont("code",     {"mono",    13.f, false, false});
    t.SetFont("small",    {"default", 11.f, false, false});

    t.cornerRadius = 4.f;
    t.borderWidth  = 1.f;
    t.padding      = 6.f;
    t.spacing      = 4.f;
    return t;
}

} // namespace UI
