#pragma once
/**
 * @file SettingsPanel.h
 * @brief PL-02 — Editor settings / preferences panel.
 *
 * Stores and serialises editor preferences:
 *   - Theme colours (accent, background, text)
 *   - Key bindings for common editor actions
 *   - AI model selection (Ollama model name)
 *   - GLFW window resolution / fullscreen
 *   - Auto-save interval
 *
 * Settings are persisted to WorkspaceState/editor_settings.json.
 */
#include <functional>
#include <map>
#include <string>
#include <cstdint>

namespace Editor {

// ── Theme ─────────────────────────────────────────────────────────────────
struct EditorTheme {
    uint32_t bgBase      = 0x1E1E1EFF;
    uint32_t bgPanel     = 0x252526FF;
    uint32_t bgTitleBar  = 0x323233FF;
    uint32_t textNormal  = 0xD4D4D4FF;
    uint32_t textAccent  = 0x569CD6FF;
    uint32_t textWarn    = 0xFFCC00FF;
    uint32_t textError   = 0xF44747FF;
    uint32_t statusBar   = 0x007ACCFF;
};

// ── Key binding ───────────────────────────────────────────────────────────
struct KeyBinding {
    std::string action;   ///< e.g. "undo", "quickSave", "quickOpen"
    int         key  = 0; ///< GLFW key code
    bool        ctrl = false;
    bool        shift = false;
    bool        alt  = false;
};

// ── Resolution ────────────────────────────────────────────────────────────
struct WindowSettings {
    int  width      = 1280;
    int  height     = 720;
    bool fullscreen = false;
    bool vsync      = true;
};

// ── Settings aggregate ────────────────────────────────────────────────────
struct EditorSettings {
    EditorTheme             theme;
    std::map<std::string, KeyBinding> keyBindings;
    WindowSettings          window;
    std::string             aiModel       = "codellama";
    std::string             ollamaEndpoint = "http://localhost:11434";
    int                     autoSaveIntervalSec = 300;
    bool                    showFPS        = true;
    bool                    showEntityCount = true;
    std::string             projectsRoot  = "Projects";
};

// ── Panel class ────────────────────────────────────────────────────────────
class SettingsPanel {
public:
    SettingsPanel();

    bool IsVisible()          const { return m_visible; }
    void Show()                     { m_visible = true; }
    void Hide()                     { m_visible = false; }
    void Toggle()                   { m_visible = !m_visible; }

    // ── Data access ───────────────────────────────────────────────────
    EditorSettings&       Settings()       { return m_settings; }
    const EditorSettings& Settings() const { return m_settings; }

    void SetTheme(const EditorTheme& theme);
    void SetKeyBinding(const std::string& action, int key,
                       bool ctrl = false, bool shift = false, bool alt = false);
    void SetAIModel(const std::string& model);
    void SetResolution(int width, int height);
    void SetOllamaEndpoint(const std::string& url);

    // ── Persistence ───────────────────────────────────────────────────
    /// Save to JSON at path (default: WorkspaceState/editor_settings.json).
    bool Save(const std::string& path = "") const;
    /// Load from JSON; returns false if file does not exist.
    bool Load(const std::string& path = "");

    // ── Change callback ───────────────────────────────────────────────
    using ChangedFn = std::function<void(const EditorSettings&)>;
    void SetChangedCallback(ChangedFn fn);
    void FireChanged();

    // ── Default bindings ──────────────────────────────────────────────
    void ResetToDefaults();

    static const std::string kDefaultPath; ///< WorkspaceState/editor_settings.json

private:
    bool        m_visible  = false;
    EditorSettings m_settings;
    ChangedFn   m_changedFn;
};

} // namespace Editor
