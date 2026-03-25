#include "UI/Themes/ThemeManager.h"
#include <algorithm>
#include <cmath>
#include <fstream>

namespace UI {

// ── ThemeManager::Init ────────────────────────────────────────────────────────

void ThemeManager::Init() {
    m_themes.clear();
    m_overrideStack.clear();
    m_activeName  = "";
    m_activeStyle = UIStyle{};
    m_transition  = ThemeTransition{};
}

// ── Theme registration ────────────────────────────────────────────────────────

void ThemeManager::RegisterTheme(const std::string& name, const UIStyle& style,
                                  const std::string& filePath) {
    ThemeEntry entry{name, style, filePath};
    m_themes[name] = std::move(entry);
    if (m_activeName.empty()) {
        m_activeName  = name;
        m_activeStyle = style;
    }
}

void ThemeManager::UnregisterTheme(const std::string& name) {
    m_themes.erase(name);
    if (m_activeName == name) m_activeName = "";
}

bool ThemeManager::HasTheme(const std::string& name) const {
    return m_themes.count(name) > 0;
}

std::vector<std::string> ThemeManager::AllThemeNames() const {
    std::vector<std::string> names;
    for (const auto& [n, _] : m_themes) names.push_back(n);
    return names;
}

// ── Built-in presets ──────────────────────────────────────────────────────────

void ThemeManager::RegisterDefaultPresets() {
    // Editor dark (default)
    RegisterTheme(ThemePreset::EditorDark,  UIStyle::Dark());
    RegisterTheme(ThemePreset::EditorLight, UIStyle::Light());

    // Ship HUD — tinted blue/cyan
    UIStyle ship = UIStyle::Dark();
    ship.textPrimary        = {180, 220, 255, 255};
    ship.accent             = {  0, 150, 220, 255};
    ship.accentHover        = {  0, 180, 240, 255};
    ship.background         = {  5,  10,  20, 220};
    ship.panelBackground    = { 10,  20,  40, 200};
    RegisterTheme(ThemePreset::ShipHUD, ship);

    // Station HUD — amber/gold tones
    UIStyle station = UIStyle::Dark();
    station.textPrimary     = {255, 220, 140, 255};
    station.accent          = {200, 140,  40, 255};
    station.accentHover     = {230, 170,  70, 255};
    station.background      = { 20,  15,   5, 220};
    station.panelBackground = { 35,  25,  10, 200};
    RegisterTheme(ThemePreset::StationHUD, station);

    // Menu screen — lighter, more opaque
    UIStyle menu = UIStyle::Light();
    menu.background         = { 20,  20,  30, 240};
    RegisterTheme(ThemePreset::MenuScreen, menu);

    // Combat HUD — red accent
    UIStyle combat = UIStyle::Dark();
    combat.accent           = {220,  50,  50, 255};
    combat.accentHover      = {240,  80,  80, 255};
    combat.accentPressed    = {180,  30,  30, 255};
    combat.textPrimary      = {255, 180, 180, 255};
    RegisterTheme(ThemePreset::CombatHUD, combat);

    // Builder mode — green/teal ghost colours
    UIStyle builder = UIStyle::Dark();
    builder.accent          = { 80, 220,  80, 255};
    builder.accentHover     = {100, 240, 100, 255};
    builder.accent          = { 40, 160,  40, 255};
    builder.textPrimary     = {180, 255, 180, 255};
    RegisterTheme(ThemePreset::BuilderMode, builder);

    SetActiveTheme(ThemePreset::EditorDark);
}

// ── Active theme ──────────────────────────────────────────────────────────────

const std::string& ThemeManager::GetActiveThemeName() const { return m_activeName; }
const UIStyle&     ThemeManager::GetActiveStyle()     const { return m_activeStyle; }

bool ThemeManager::SetActiveTheme(const std::string& name) {
    auto it = m_themes.find(name);
    if (it == m_themes.end()) return false;
    m_activeName  = name;
    m_activeStyle = it->second.style;
    m_transition.active = false;
    if (m_onThemeChanged) m_onThemeChanged(name, m_activeStyle);
    return true;
}

bool ThemeManager::TransitionTo(const std::string& targetName, float duration) {
    auto it = m_themes.find(targetName);
    if (it == m_themes.end()) return false;
    m_transition.from     = m_activeStyle;
    m_transition.to       = it->second.style;
    m_transition.duration = std::max(0.001f, duration);
    m_transition.elapsed  = 0.0f;
    m_transition.active   = true;
    // Pre-set the target name so queries see it immediately
    m_activeName = targetName;
    return true;
}

// ── Per-context override ──────────────────────────────────────────────────────

void ThemeManager::PushOverride(const std::string& contextTag, const UIStyle& partial) {
    m_overrideStack.emplace_back(contextTag, partial);
}

void ThemeManager::PopOverride(const std::string& contextTag) {
    for (auto it = m_overrideStack.rbegin(); it != m_overrideStack.rend(); ++it) {
        if (it->first == contextTag) {
            m_overrideStack.erase((it + 1).base());
            return;
        }
    }
}

UIStyle ThemeManager::GetEffectiveStyle() const {
    UIStyle effective = m_activeStyle;
    for (const auto& [tag, override] : m_overrideStack) {
        // Merge: only non-default (non-zero-alpha) colours override
        if (override.accent.a > 0)          effective.accent           = override.accent;
        if (override.textPrimary.a > 0)     effective.textPrimary      = override.textPrimary;
        if (override.background.a > 0)      effective.background       = override.background;
        if (override.panelBackground.a > 0) effective.panelBackground  = override.panelBackground;
    }
    return effective;
}

// ── Hot-reload ────────────────────────────────────────────────────────────────

bool ThemeManager::HotReloadTheme(const std::string& name) {
    auto it = m_themes.find(name);
    if (it == m_themes.end() || it->second.filePath.empty()) return false;
    std::ifstream f(it->second.filePath);
    if (!f.is_open()) return false;
    // Full JSON parsing would go here; for now just trigger the callback
    // to signal that the theme was "reloaded".
    if (m_activeName == name && m_onThemeChanged)
        m_onThemeChanged(name, m_activeStyle);
    return true;
}

// ── Tick ─────────────────────────────────────────────────────────────────────

void ThemeManager::Tick(float dt) {
    if (!m_transition.active) return;
    m_transition.elapsed += dt;
    float t = std::min(m_transition.elapsed / m_transition.duration, 1.0f);
    m_activeStyle = Lerp(m_transition.from, m_transition.to, t);
    if (t >= 1.0f) {
        m_transition.active = false;
        if (m_onThemeChanged) m_onThemeChanged(m_activeName, m_activeStyle);
    }
}

// ── Callback ─────────────────────────────────────────────────────────────────

void ThemeManager::SetThemeChangedCallback(ThemeChangedFn fn) { m_onThemeChanged = std::move(fn); }

// ── Private helpers ───────────────────────────────────────────────────────────

UIColor ThemeManager::LerpColor(UIColor a, UIColor b, float t) const {
    auto lerp = [](uint8_t x, uint8_t y, float f) -> uint8_t {
        return (uint8_t)(x + (int)(f * (y - x)));
    };
    return { lerp(a.r, b.r, t), lerp(a.g, b.g, t),
             lerp(a.b, b.b, t), lerp(a.a, b.a, t) };
}

UIStyle ThemeManager::Lerp(const UIStyle& a, const UIStyle& b, float t) const {
    UIStyle s = a;
    s.background       = LerpColor(a.background,       b.background,       t);
    s.panelBackground  = LerpColor(a.panelBackground,  b.panelBackground,  t);
    s.textPrimary      = LerpColor(a.textPrimary,      b.textPrimary,      t);
    s.textSecondary    = LerpColor(a.textSecondary,    b.textSecondary,    t);
    s.accent           = LerpColor(a.accent,           b.accent,           t);
    s.accentHover      = LerpColor(a.accentHover,      b.accentHover,      t);
    s.border           = LerpColor(a.border,           b.border,           t);
    s.statusOk         = LerpColor(a.statusOk,         b.statusOk,         t);
    s.statusWarning    = LerpColor(a.statusWarning,    b.statusWarning,    t);
    s.statusError      = LerpColor(a.statusError,      b.statusError,      t);
    return s;
}

} // namespace UI
