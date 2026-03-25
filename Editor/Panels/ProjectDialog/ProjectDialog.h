#pragma once
/**
 * @file ProjectDialog.h
 * @brief EI-10 — Project new/open dialog displayed on editor startup or via
 *        "File > New Project" / "File > Open Project…".
 *
 * Rendered as a modal overlay by EditorRenderer.  When the user selects or
 * creates a project the dialog invokes the registered callback and hides
 * itself.
 */
#include <functional>
#include <string>
#include <vector>

namespace Editor {

enum class ProjectDialogMode { NewProject, OpenProject };

struct ProjectInfo {
    std::string name;       ///< Display name
    std::string rootPath;   ///< Absolute path to the project root directory
};

class ProjectDialog {
public:
    // ── Configuration ────────────────────────────────────────────────────
    /// Show the dialog in New or Open mode.
    void Show(ProjectDialogMode mode = ProjectDialogMode::OpenProject);
    void Hide();
    bool IsVisible() const { return m_visible; }

    // ── Callbacks ────────────────────────────────────────────────────────
    using ProjectSelectedFn = std::function<void(const ProjectInfo&)>;
    void SetProjectSelectedCallback(ProjectSelectedFn fn);

    // ── Called by the renderer every frame when IsVisible() ──────────────
    /// Returns true while the dialog is still open; false once dismissed.
    bool Update(double mouseX, double mouseY, bool leftClicked);

    // ── Recent projects list ─────────────────────────────────────────────
    void SetProjectsRoot(const std::string& basePath);
    void ScanRecentProjects();
    const std::vector<ProjectInfo>& RecentProjects() const { return m_recent; }

    // ── New project creation ─────────────────────────────────────────────
    /// Scaffold a new project directory with default scene + Assets/ folder.
    static bool CreateProjectScaffold(const std::string& rootPath,
                                      const std::string& projectName);

private:
    bool               m_visible  = false;
    ProjectDialogMode  m_mode     = ProjectDialogMode::OpenProject;
    std::string        m_basePath;
    std::string        m_newName;         // typed name for New Project
    std::string        m_typedPath;       // typed path for Open Project
    std::vector<ProjectInfo> m_recent;
    int                m_hoveredRecent = -1;
    ProjectSelectedFn  m_selectedFn;
};

} // namespace Editor
