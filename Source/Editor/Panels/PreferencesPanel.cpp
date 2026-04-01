#include "Editor/Panels/PreferencesPanel.h"
#include "Core/Logging/Log.h"
#include <fstream>
#include <sstream>
#include <string>

namespace NF::Editor {

// ---------------------------------------------------------------------------
// Minimal INI helpers (no external dependency)
// ---------------------------------------------------------------------------

static std::string ThemeToString(EditorTheme t)
{
    switch (t) {
        case EditorTheme::Light:       return "Light";
        case EditorTheme::HighContrast: return "HighContrast";
        default:                       return "Dark";
    }
}

static EditorTheme ThemeFromString(const std::string& s)
{
    if (s == "Light")        return EditorTheme::Light;
    if (s == "HighContrast") return EditorTheme::HighContrast;
    return EditorTheme::Dark;
}

// ---------------------------------------------------------------------------

bool PreferencesPanel::Load(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '[') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        if (key == "theme")                m_Data.theme               = ThemeFromString(val);
        else if (key == "fpsLimit")        m_Data.fpsLimit            = static_cast<uint32_t>(std::stoul(val));
        else if (key == "autosaveEnabled") m_Data.autosaveEnabled     = (val == "1" || val == "true");
        else if (key == "autosaveInterval") m_Data.autosaveIntervalSec = static_cast<uint32_t>(std::stoul(val));
        else if (key == "showGrid")        m_Data.showGrid            = (val == "1" || val == "true");
        else if (key == "uiScale")         m_Data.uiScale             = std::stof(val);
        else if (key == "recentProjectsDir") m_Data.recentProjectsDir = val;
    }
    Logger::Log(LogLevel::Info, "Preferences", "Loaded from " + path);
    return true;
}

bool PreferencesPanel::Save(const std::string& path) const
{
    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << "[editor]\n";
    f << "theme="                << ThemeToString(m_Data.theme)            << "\n";
    f << "fpsLimit="             << m_Data.fpsLimit                        << "\n";
    f << "autosaveEnabled="      << (m_Data.autosaveEnabled ? "1" : "0")  << "\n";
    f << "autosaveInterval="     << m_Data.autosaveIntervalSec             << "\n";
    f << "showGrid="             << (m_Data.showGrid ? "1" : "0")         << "\n";
    f << "uiScale="              << m_Data.uiScale                         << "\n";
    f << "recentProjectsDir="    << m_Data.recentProjectsDir               << "\n";

    Logger::Log(LogLevel::Info, "Preferences", "Saved to " + path);
    return true;
}

void PreferencesPanel::Update(float dt)
{
    if (!m_Data.autosaveEnabled) return;
    m_AutosaveTimer += dt;
    if (m_AutosaveTimer >= static_cast<float>(m_Data.autosaveIntervalSec)) {
        m_AutosaveTimer = 0.f;
        Logger::Log(LogLevel::Info, "Preferences", "Auto-save triggered");
        Save("editor_prefs.ini");
    }
}

void PreferencesPanel::Draw()
{
    if (!m_Open) return;
    Logger::Log(LogLevel::Trace, "Preferences", "Draw – PreferencesPanel");
    // Concrete UI drawing is performed by the platform UI layer.
}

} // namespace NF::Editor
