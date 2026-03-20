#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "UI/Widgets/Widget.h"

namespace UI {

// ──────────────────────────────────────────────────────────────
// ThemeColor — semantic color roles
// ──────────────────────────────────────────────────────────────

enum class ThemeColor : uint32_t {
    Background = 0,
    Surface,
    Primary,
    Secondary,
    Accent,
    Text,
    TextMuted,
    Border,
    Error,
    Warning,
    Success,
    ButtonBg,
    ButtonHover,
    ButtonPressed,
    InputBg,
    InputBorder,
    PanelBg,
    OverlayBg,
    ScrollBar,
    _Count
};

// ──────────────────────────────────────────────────────────────
// ThemeFont
// ──────────────────────────────────────────────────────────────

struct ThemeFont {
    std::string name   = "default";
    float       size   = 14.0f;
    bool        bold   = false;
    bool        italic = false;
};

// ──────────────────────────────────────────────────────────────
// Theme
// ──────────────────────────────────────────────────────────────

struct Theme {
    std::string name;
    std::unordered_map<ThemeColor, uint32_t>    colors;  // RGBA packed
    std::unordered_map<std::string, ThemeFont>  fonts;   // role → font
    float cornerRadius = 4.0f;
    float borderWidth  = 1.0f;
    float padding      = 6.0f;
    float spacing      = 4.0f;

    // Colour accessors
    uint32_t GetColor(ThemeColor c) const;
    void     SetColor(ThemeColor c, uint32_t rgba);

    // Font accessors (roles: "default", "heading", "code", "small")
    ThemeFont  GetFont(const std::string& role) const;
    void       SetFont(const std::string& role, ThemeFont font);

    // Apply theme fields to a WidgetStyle for a given widget type
    void ApplyTo(WidgetStyle& style, WidgetType type) const;
};

// ──────────────────────────────────────────────────────────────
// ThemeManager — non-singleton; pass by reference
// ──────────────────────────────────────────────────────────────

class ThemeManager {
public:
    ThemeManager();

    void RegisterTheme(Theme theme);
    bool SetActiveTheme(const std::string& name);

    const Theme& GetActiveTheme() const;
    const Theme* GetTheme(const std::string& name) const;

    std::vector<std::string> ThemeNames() const;

    // Built-in factory themes
    static Theme MakeDarkTheme();
    static Theme MakeLightTheme();

private:
    std::unordered_map<std::string, Theme> m_themes;
    std::string                            m_activeName;
};

} // namespace UI
