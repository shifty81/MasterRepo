#pragma once
/**
 * @file ThemeManager.h
 * @brief Full UI theme management — apply UIStyle live with hot-reload support.
 *
 * Extends the existing UI::Theme and UI::UIStyle with:
 *   - Named theme registry (register any number of themes by string key)
 *   - Active theme switching with instant propagation to all widgets
 *   - Game-context presets: ShipHUD, StationHUD, MenuScreen, EditorDark, etc.
 *   - Per-context override: apply a partial style override on top of the base theme
 *   - Hot-reload: load a theme from a JSON/style file at runtime
 *   - Interpolation: smooth alpha-blend between two themes over a duration
 *
 * Usage:
 * @code
 *   ThemeManager tm;
 *   tm.Init();
 *   tm.RegisterTheme("ship_hud",    UIStyle::ShipHUD());
 *   tm.RegisterTheme("station_hud", UIStyle::StationHUD());
 *   tm.SetActiveTheme("ship_hud");
 *   // … entering station …
 *   tm.TransitionTo("station_hud", 0.5f);  // 0.5s blend
 * @endcode
 */

#include "UI/Themes/UIStyle.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace UI {

// ── Game-context preset names ─────────────────────────────────────────────────

namespace ThemePreset {
inline constexpr const char* EditorDark    = "editor_dark";
inline constexpr const char* EditorLight   = "editor_light";
inline constexpr const char* ShipHUD       = "ship_hud";
inline constexpr const char* StationHUD    = "station_hud";
inline constexpr const char* MenuScreen    = "menu_screen";
inline constexpr const char* CombatHUD     = "combat_hud";
inline constexpr const char* BuilderMode   = "builder_mode";
}

// ── Theme entry ───────────────────────────────────────────────────────────────

struct ThemeEntry {
    std::string name;
    UIStyle     style;
    std::string filePath;    ///< optional — path to JSON style file for hot-reload
};

// ── Theme transition state ────────────────────────────────────────────────────

struct ThemeTransition {
    UIStyle  from;
    UIStyle  to;
    float    duration{0.5f};
    float    elapsed{0.0f};
    bool     active{false};
};

using ThemeChangedFn = std::function<void(const std::string& newThemeName,
                                          const UIStyle& newStyle)>;

// ── ThemeManager ──────────────────────────────────────────────────────────────

class ThemeManager {
public:
    void Init();

    // ── Theme registration ────────────────────────────────────────────────
    void RegisterTheme(const std::string& name, const UIStyle& style,
                        const std::string& filePath = "");
    void UnregisterTheme(const std::string& name);
    bool HasTheme(const std::string& name) const;
    std::vector<std::string> AllThemeNames() const;

    // ── Built-in presets ──────────────────────────────────────────────────
    /// Register the default set of game-context presets.
    void RegisterDefaultPresets();

    // ── Active theme ──────────────────────────────────────────────────────
    const std::string& GetActiveThemeName() const;
    const UIStyle&     GetActiveStyle()     const;

    /// Immediately switch to theme (no transition).
    bool SetActiveTheme(const std::string& name);

    /// Smooth transition from current to target theme over `duration` seconds.
    bool TransitionTo(const std::string& targetName, float duration = 0.5f);

    // ── Per-context override ──────────────────────────────────────────────
    /// Push a partial override on top of the active theme.
    void PushOverride(const std::string& contextTag, const UIStyle& partial);
    void PopOverride(const std::string& contextTag);

    /// Effective style = active theme merged with all active overrides.
    UIStyle GetEffectiveStyle() const;

    // ── Hot-reload ────────────────────────────────────────────────────────
    /// Reload the theme's style from its registered filePath.
    bool HotReloadTheme(const std::string& name);

    // ── Tick ──────────────────────────────────────────────────────────────
    void Tick(float dt);

    // ── Callback ─────────────────────────────────────────────────────────
    void SetThemeChangedCallback(ThemeChangedFn fn);

private:
    UIStyle Lerp(const UIStyle& a, const UIStyle& b, float t) const;
    UIColor LerpColor(UIColor a, UIColor b, float t) const;

    std::unordered_map<std::string, ThemeEntry> m_themes;
    std::string     m_activeName;
    UIStyle         m_activeStyle;
    ThemeTransition m_transition;

    std::vector<std::pair<std::string, UIStyle>> m_overrideStack;

    ThemeChangedFn m_onThemeChanged;
};

} // namespace UI
