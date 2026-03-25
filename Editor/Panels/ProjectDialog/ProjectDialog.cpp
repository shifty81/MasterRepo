#include "Editor/Panels/ProjectDialog/ProjectDialog.h"
#include "Engine/Core/Logger.h"
#include <filesystem>
#include <fstream>

namespace Editor {

namespace {
/// Escape a string for safe inclusion in a JSON value (double-quoted context).
std::string JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;     break;
        }
    }
    return out;
}
} // anonymous namespace

// ──────────────────────────────────────────────────────────────────────────
void ProjectDialog::Show(ProjectDialogMode mode) {
    m_mode    = mode;
    m_visible = true;
    m_newName.clear();
    m_typedPath.clear();
    ScanRecentProjects();
}

void ProjectDialog::Hide() { m_visible = false; }

void ProjectDialog::SetProjectSelectedCallback(ProjectSelectedFn fn) {
    m_selectedFn = std::move(fn);
}

void ProjectDialog::SetProjectsRoot(const std::string& basePath) {
    m_basePath = basePath;
}

// ──────────────────────────────────────────────────────────────────────────
void ProjectDialog::ScanRecentProjects() {
    m_recent.clear();
    if (m_basePath.empty()) return;
    if (!std::filesystem::exists(m_basePath)) return;

    for (const auto& entry :
            std::filesystem::directory_iterator(m_basePath)) {
        if (!entry.is_directory()) continue;
        // A valid project directory must contain an Assets/ or Scenes/ sub-folder
        auto p = entry.path();
        if (std::filesystem::exists(p / "Assets") ||
            std::filesystem::exists(p / "Scenes"))
        {
            ProjectInfo pi;
            pi.name     = p.filename().string();
            pi.rootPath = p.string();
            m_recent.push_back(std::move(pi));
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
bool ProjectDialog::Update(double /*mouseX*/, double /*mouseY*/,
                           bool /*leftClicked*/) {
    // Rendering is handled by EditorRenderer::DrawProjectDialog (not yet
    // integrated into the render pipeline — placeholder stub).
    return m_visible;
}

// ──────────────────────────────────────────────────────────────────────────
// static
bool ProjectDialog::CreateProjectScaffold(const std::string& rootPath,
                                          const std::string& projectName) {
    namespace fs = std::filesystem;
    try {
        fs::path root(rootPath);
        fs::create_directories(root / "Assets" / "Textures");
        fs::create_directories(root / "Assets" / "Meshes");
        fs::create_directories(root / "Assets" / "Materials");
        fs::create_directories(root / "Scenes");
        fs::create_directories(root / "Scripts");
        fs::create_directories(root / "Recipes");

        // Write a minimal default.scene placeholder
        std::ofstream scene(root / "Scenes" / "default.scene");
        scene << "{\n"
              << "  \"project\": \"" << JsonEscape(projectName) << "\",\n"
              << "  \"version\": 1,\n"
              << "  \"entities\": []\n"
              << "}\n";

        // Write a minimal project.json
        std::ofstream proj(root / "project.json");
        proj << "{\n"
             << "  \"name\": \"" << JsonEscape(projectName) << "\",\n"
             << "  \"engine\": \"AtlasEditor\",\n"
             << "  \"version\": \"0.1\"\n"
             << "}\n";

        Engine::Core::Logger::Info("Project scaffold created: " + rootPath);
        return true;
    } catch (const std::exception& e) {
        Engine::Core::Logger::Error(
            std::string("Failed to create project scaffold: ") + e.what());
        return false;
    }
}

} // namespace Editor
