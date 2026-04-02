#pragma once
#include <string>
#include <vector>

namespace NF {

/// @brief Data loaded from `Config/novaforge.project.json`.
///
/// This is the project-level manifest that defines the project identity,
/// standard content/config/save paths, active phase list, and locked
/// design rules.  Both the editor and game bootstrap load it early in
/// their startup sequence so that every subsystem can resolve paths and
/// query project-level settings without ad-hoc path logic.
struct ProjectManifest {
    // -- project block --
    std::string ProjectName        = "NovaForge";
    std::string ProjectType        = "standalone-game-repo";
    std::string ProjectVersion     = "0.1.0-reset";
    std::string DefaultWorld       = "DevWorld";
    std::string DefaultStartupMode = "Editor";

    // -- paths block --
    std::string ContentRoot = "Content";
    std::string ConfigRoot  = "Config";
    std::string SaveRoot    = "Saved";
    std::string LogRoot     = "Saved/Logs";

    // -- phases array --
    std::vector<std::string> Phases;

    // -- rules block --
    bool VoxelAuthoritative      = true;
    bool EditorShipsWithGame     = false;
    bool AllowSuiteFeaturesInRepo = false;

    // -------------------------------------------------------------------------
    // I/O
    // -------------------------------------------------------------------------

    /// @brief Load the manifest from a JSON file on disk.
    /// @param path Path to `novaforge.project.json`.
    /// @return True if the file was opened and all required fields parsed.
    bool LoadFromFile(const std::string& path);

    /// @brief Returns true when the manifest has been loaded and contains
    ///        at least a non-empty project name and version.
    [[nodiscard]] bool IsValid() const noexcept;

    /// @brief Log the manifest contents to the engine log.
    void LogSummary() const;
};

} // namespace NF
