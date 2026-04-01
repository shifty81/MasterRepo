#pragma once
#include <cstdint>
#include <string>

namespace NF::Editor {

/// @brief Editor colour theme.
enum class EditorTheme { Dark, Light, HighContrast };

/// @brief Persistent editor preferences stored and loaded on disk.
///
/// Values are intentionally plain data so they can be trivially
/// serialised to a simple INI-style file via Save()/Load().
struct PreferenceData {
    EditorTheme theme{EditorTheme::Dark};   ///< UI colour theme.
    uint32_t    fpsLimit{60};               ///< Renderer target frame rate (0 = unlimited).
    bool        autosaveEnabled{true};      ///< Enable periodic project auto-save.
    uint32_t    autosaveIntervalSec{300};   ///< Seconds between auto-save ticks.
    bool        showGrid{true};             ///< Render the viewport grid.
    float       uiScale{1.0f};             ///< Global UI DPI scale factor.
    std::string recentProjectsDir{};        ///< Last-used projects directory path.
};

/// @brief Editor preferences panel.
///
/// Displays editable settings and persists them to disk when the
/// user clicks "Apply".
class PreferencesPanel {
public:
    /// @brief Load preferences from @p path (INI format).
    /// @return True when the file existed and was parsed successfully.
    bool Load(const std::string& path);

    /// @brief Save current preferences to @p path.
    /// @return True on success.
    bool Save(const std::string& path) const;

    /// @brief Return a read-only view of the current preference data.
    [[nodiscard]] const PreferenceData& GetData() const noexcept { return m_Data; }

    /// @brief Return a mutable reference to the preference data (for programmatic edits).
    [[nodiscard]] PreferenceData& GetData() noexcept { return m_Data; }

    /// @brief Advance panel state (handles autosave timer).
    void Update(float dt);

    /// @brief Draw the preferences panel UI.
    void Draw();

    /// @brief Returns true while the panel should be visible.
    [[nodiscard]] bool IsOpen() const noexcept { return m_Open; }

    /// @brief Show or hide the panel.
    void SetOpen(bool open) noexcept { m_Open = open; }

private:
    PreferenceData m_Data;
    bool           m_Open{false};
    float          m_AutosaveTimer{0.f};
    bool           m_Dirty{false};       ///< True when unsaved changes exist.
};

} // namespace NF::Editor
