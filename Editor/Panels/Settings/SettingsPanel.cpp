#include "Editor/Panels/Settings/SettingsPanel.h"
#include "Engine/Core/Logger.h"
#include <fstream>
#include <sstream>
#include <filesystem>

// GLFW key constants (duplicated to avoid GLFW header dependency)
#ifndef GLFW_KEY_Z
static constexpr int K_Z     = 90;
static constexpr int K_Y     = 89;
static constexpr int K_S     = 83;
static constexpr int K_O     = 79;
static constexpr int K_P     = 80;
static constexpr int K_F5    = 294;
static constexpr int K_F9    = 297;
static constexpr int K_F2    = 291;
#else
static constexpr int K_Z     = GLFW_KEY_Z;
static constexpr int K_Y     = GLFW_KEY_Y;
static constexpr int K_S     = GLFW_KEY_S;
static constexpr int K_O     = GLFW_KEY_O;
static constexpr int K_P     = GLFW_KEY_P;
static constexpr int K_F5    = GLFW_KEY_F5;
static constexpr int K_F9    = GLFW_KEY_F9;
static constexpr int K_F2    = GLFW_KEY_F2;
#endif

namespace Editor {

const std::string SettingsPanel::kDefaultPath = "WorkspaceState/editor_settings.json";

SettingsPanel::SettingsPanel() {
    ResetToDefaults();
}

void SettingsPanel::ResetToDefaults() {
    m_settings = EditorSettings{};  // zero-init + member defaults
    // Key bindings
    m_settings.keyBindings["undo"]        = {"undo",       K_Z,  true,  false, false};
    m_settings.keyBindings["redo"]        = {"redo",       K_Y,  true,  false, false};
    m_settings.keyBindings["saveScene"]   = {"saveScene",  K_S,  true,  false, false};
    m_settings.keyBindings["openScene"]   = {"openScene",  K_O,  true,  false, false};
    m_settings.keyBindings["quickOpen"]   = {"quickOpen",  K_P,  true,  false, false};
    m_settings.keyBindings["quickSave"]   = {"quickSave",  K_F5, false, false, false};
    m_settings.keyBindings["quickLoad"]   = {"quickLoad",  K_F9, false, false, false};
    m_settings.keyBindings["builderMode"] = {"builderMode",K_F2, false, false, false};
}

void SettingsPanel::SetTheme(const EditorTheme& theme) {
    m_settings.theme = theme;
    FireChanged();
}

void SettingsPanel::SetKeyBinding(const std::string& action, int key,
                                   bool ctrl, bool shift, bool alt) {
    m_settings.keyBindings[action] = {action, key, ctrl, shift, alt};
    FireChanged();
}

void SettingsPanel::SetAIModel(const std::string& model) {
    m_settings.aiModel = model;
    FireChanged();
}

void SettingsPanel::SetResolution(int width, int height) {
    m_settings.window.width  = width;
    m_settings.window.height = height;
    FireChanged();
}

void SettingsPanel::SetOllamaEndpoint(const std::string& url) {
    m_settings.ollamaEndpoint = url;
    FireChanged();
}

void SettingsPanel::SetChangedCallback(ChangedFn fn) {
    m_changedFn = std::move(fn);
}

void SettingsPanel::FireChanged() {
    if (m_changedFn) m_changedFn(m_settings);
}

// ── Persistence ────────────────────────────────────────────────────────────
bool SettingsPanel::Save(const std::string& path) const {
    const std::string& outPath = path.empty() ? kDefaultPath : path;
    std::filesystem::create_directories(
        std::filesystem::path(outPath).parent_path());

    std::ofstream f(outPath);
    if (!f.is_open()) {
        Engine::Core::Logger::Error("SettingsPanel: cannot write " + outPath);
        return false;
    }

    auto hex8 = [](uint32_t v) -> std::string {
        char buf[12];
        std::snprintf(buf, sizeof(buf), "%08X", v);
        return buf;
    };

    f << "{\n";
    f << "  \"theme\": {\n"
      << "    \"bgBase\": \""     << hex8(m_settings.theme.bgBase)     << "\",\n"
      << "    \"bgPanel\": \""    << hex8(m_settings.theme.bgPanel)    << "\",\n"
      << "    \"textNormal\": \"" << hex8(m_settings.theme.textNormal) << "\",\n"
      << "    \"textAccent\": \"" << hex8(m_settings.theme.textAccent) << "\",\n"
      << "    \"statusBar\": \""  << hex8(m_settings.theme.statusBar)  << "\"\n"
      << "  },\n";
    f << "  \"window\": {\n"
      << "    \"width\": "      << m_settings.window.width  << ",\n"
      << "    \"height\": "     << m_settings.window.height << ",\n"
      << "    \"fullscreen\": " << (m_settings.window.fullscreen ? "true" : "false") << ",\n"
      << "    \"vsync\": "      << (m_settings.window.vsync ? "true" : "false") << "\n"
      << "  },\n";
    f << "  \"aiModel\": \""          << m_settings.aiModel          << "\",\n";
    f << "  \"ollamaEndpoint\": \""   << m_settings.ollamaEndpoint   << "\",\n";
    f << "  \"autoSaveInterval\": "   << m_settings.autoSaveIntervalSec << ",\n";
    f << "  \"showFPS\": "            << (m_settings.showFPS ? "true" : "false") << ",\n";
    f << "  \"projectsRoot\": \""     << m_settings.projectsRoot     << "\",\n";
    f << "  \"keyBindings\": {\n";
    bool first = true;
    for (const auto& [name, kb] : m_settings.keyBindings) {
        if (!first) f << ",\n";
        f << "    \"" << name << "\": {\"key\":" << kb.key
          << ",\"ctrl\":" << (kb.ctrl ? "true" : "false")
          << ",\"shift\":"<< (kb.shift ? "true" : "false")
          << ",\"alt\":"  << (kb.alt  ? "true" : "false") << "}";
        first = false;
    }
    f << "\n  }\n}\n";

    Engine::Core::Logger::Info("SettingsPanel: saved to " + outPath);
    return true;
}

bool SettingsPanel::Load(const std::string& path) {
    const std::string& inPath = path.empty() ? kDefaultPath : path;
    std::ifstream f(inPath);
    if (!f.is_open()) return false;  // first run — use defaults

    // Minimal parsing: just extract known string fields
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    auto extractStr = [&](const std::string& key) -> std::string {
        auto pos = content.find("\"" + key + "\":");
        if (pos == std::string::npos) return {};
        auto start = content.find('"', pos + key.size() + 3);
        if (start == std::string::npos) return {};
        auto end = content.find('"', start + 1);
        return end != std::string::npos ? content.substr(start + 1, end - start - 1) : "";
    };
    auto extractInt = [&](const std::string& key, int def) -> int {
        auto pos = content.find("\"" + key + "\":");
        if (pos == std::string::npos) return def;
        pos += key.size() + 3;
        while (pos < content.size() && (content[pos]==' '||content[pos]=='\t')) ++pos;
        // Parse until a JSON delimiter: comma, newline, or closing brace
        auto end = content.find_first_of(",\n}", pos);
        if (end == std::string::npos) end = content.size();
        try { return std::stoi(content.substr(pos, end - pos)); } catch (...) { return def; }
    };

    auto aiModel = extractStr("aiModel");
    if (!aiModel.empty()) m_settings.aiModel = aiModel;
    auto endpoint = extractStr("ollamaEndpoint");
    if (!endpoint.empty()) m_settings.ollamaEndpoint = endpoint;
    m_settings.window.width  = extractInt("width",  1280);
    m_settings.window.height = extractInt("height", 720);
    m_settings.autoSaveIntervalSec = extractInt("autoSaveInterval", 300);

    Engine::Core::Logger::Info("SettingsPanel: loaded from " + inPath);
    return true;
}

} // namespace Editor
