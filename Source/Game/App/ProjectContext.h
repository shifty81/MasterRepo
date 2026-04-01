#pragma once
#include <string>

namespace NF::Game {

/// @brief Configuration for constructing a ProjectContext.
///
/// All sub-paths are relative to @c repoRoot unless they are already
/// absolute.  Defaults match the standard MasterRepo layout.
struct ProjectContextConfig {
    std::string projectId   = "novaforge";   ///< Stable project identifier.
    std::string displayName = "NovaForge";   ///< Human-readable name.
    std::string version     = "0.1.0";       ///< Semantic version string.

    std::string repoRoot;                    ///< Absolute path to repository root.

    // Sub-paths (relative to repoRoot)
    std::string dataRoot    = "Source/Game/Gameplay/Data";
    std::string contentRoot = "Content";
    std::string docsRoot    = "Docs";
    std::string scriptsRoot = "Scripts";
    std::string testsRoot   = "Tests";
    std::string gameRoot    = "Source/Game";

    bool editorMode = false; ///< True when running inside the editor.
    bool debugMode  = true;  ///< True when diagnostic output is enabled.
};

/// @brief Holds all project-level path, metadata, and config knowledge.
///
/// Constructed once at bootstrap from a @c ProjectContextConfig; callers
/// treat the resulting object as read-only.  No engine or gameplay headers
/// are included here.
class ProjectContext {
public:
    /// @brief Construct from an explicit config.
    explicit ProjectContext(const ProjectContextConfig& config);

    /// @brief Create a context with default NovaForge paths given a repo root.
    /// @param repoRoot Absolute path to the repository root directory.
    static ProjectContext CreateDefault(const std::string& repoRoot);

    // -------------------------------------------------------------------------
    // Identity
    // -------------------------------------------------------------------------

    /// @brief Returns the stable project identifier (e.g. "novaforge").
    [[nodiscard]] const std::string& ProjectId()   const noexcept;
    /// @brief Returns the display name (e.g. "NovaForge").
    [[nodiscard]] const std::string& DisplayName() const noexcept;
    /// @brief Returns the semantic version string (e.g. "0.1.0").
    [[nodiscard]] const std::string& Version()     const noexcept;

    // -------------------------------------------------------------------------
    // Resolved absolute paths
    // -------------------------------------------------------------------------

    [[nodiscard]] std::string RepoRoot()    const;
    [[nodiscard]] std::string DataRoot()    const;
    [[nodiscard]] std::string ContentRoot() const;
    [[nodiscard]] std::string DocsRoot()    const;
    [[nodiscard]] std::string ScriptsRoot() const;
    [[nodiscard]] std::string TestsRoot()   const;
    [[nodiscard]] std::string GameRoot()    const;

    // -------------------------------------------------------------------------
    // Flags
    // -------------------------------------------------------------------------

    [[nodiscard]] bool IsEditorMode() const noexcept;
    [[nodiscard]] bool IsDebugMode()  const noexcept;

    // -------------------------------------------------------------------------
    // Validation
    // -------------------------------------------------------------------------

    /// @brief Returns true when repoRoot, projectId, and version are all non-empty.
    [[nodiscard]] bool IsValid() const noexcept;

private:
    ProjectContextConfig m_Config;

    /// @brief Join @p base and @p relative into a single path string.
    static std::string JoinPath(const std::string& base,
                                const std::string& relative);
};

} // namespace NF::Game
