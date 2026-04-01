#pragma once
#include "Game/App/ProjectContext.h"
#include "Game/App/Session.h"
#include <memory>
#include <string>

namespace NF::Game {

/// @brief Result returned by Bootstrap::Run().
struct BootstrapResult {
    bool        Success = false; ///< True when all startup steps succeeded.
    std::string Message;         ///< Human-readable status or error description.
};

/// @brief Input parameters for Bootstrap::Run().
struct BootstrapConfig {
    /// Absolute path to the repository root; used for path resolution.
    std::string RepoRoot;

    /// Optional override for the project context; leave default to use
    /// standard NovaForge paths derived from @c RepoRoot.
    ProjectContextConfig ContextConfig;
};

/// @brief Orchestrates the game startup sequence.
///
/// Responsibilities:
///  - Build and validate the @c ProjectContext from @c BootstrapConfig.
///  - Create the active @c Session.
///  - Provide a matching @c Shutdown() that reverses all of the above.
///
/// Bootstrap must not contain gameplay logic.
class Bootstrap {
public:
    Bootstrap();
    ~Bootstrap();

    Bootstrap(const Bootstrap&)            = delete;
    Bootstrap& operator=(const Bootstrap&) = delete;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// @brief Execute the full startup sequence.
    /// @param config Configuration parameters.
    /// @return Result indicating success or describing the failure.
    BootstrapResult Run(const BootstrapConfig& config);

    /// @brief Shut down all services started by Run().
    void Shutdown();

    // -------------------------------------------------------------------------
    // Accessors (valid only after a successful Run())
    // -------------------------------------------------------------------------

    /// @brief Returns the project context, or nullptr if not yet bootstrapped.
    [[nodiscard]] const ProjectContext* GetProjectContext() const noexcept;

    /// @brief Returns the active session, or nullptr if not bootstrapped.
    [[nodiscard]] const Session* GetSession() const noexcept;

    /// @brief Returns true while the bootstrap is in the running state.
    [[nodiscard]] bool IsRunning() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

} // namespace NF::Game
