#include "Game/App/ProjectContext.h"
#include <cctype>
#include <stdexcept>

namespace NF::Game {

// ----------------------------------------------------------------------------
// Construction
// ----------------------------------------------------------------------------

ProjectContext::ProjectContext(const ProjectContextConfig& config)
    : m_Config(config)
{
}

ProjectContext ProjectContext::CreateDefault(const std::string& repoRoot)
{
    if (repoRoot.empty())
        throw std::invalid_argument(
            "ProjectContext::CreateDefault: repoRoot must not be empty");

    ProjectContextConfig cfg;
    cfg.repoRoot = repoRoot;
    return ProjectContext(cfg);
}

// ----------------------------------------------------------------------------
// Identity
// ----------------------------------------------------------------------------

const std::string& ProjectContext::ProjectId()   const noexcept { return m_Config.projectId; }
const std::string& ProjectContext::DisplayName() const noexcept { return m_Config.displayName; }
const std::string& ProjectContext::Version()     const noexcept { return m_Config.version; }

// ----------------------------------------------------------------------------
// Resolved paths
// ----------------------------------------------------------------------------

std::string ProjectContext::RepoRoot()    const { return m_Config.repoRoot; }
std::string ProjectContext::DataRoot()    const { return JoinPath(m_Config.repoRoot, m_Config.dataRoot); }
std::string ProjectContext::ContentRoot() const { return JoinPath(m_Config.repoRoot, m_Config.contentRoot); }
std::string ProjectContext::DocsRoot()    const { return JoinPath(m_Config.repoRoot, m_Config.docsRoot); }
std::string ProjectContext::ScriptsRoot() const { return JoinPath(m_Config.repoRoot, m_Config.scriptsRoot); }
std::string ProjectContext::TestsRoot()   const { return JoinPath(m_Config.repoRoot, m_Config.testsRoot); }
std::string ProjectContext::GameRoot()    const { return JoinPath(m_Config.repoRoot, m_Config.gameRoot); }

// ----------------------------------------------------------------------------
// Flags
// ----------------------------------------------------------------------------

bool ProjectContext::IsEditorMode() const noexcept { return m_Config.editorMode; }
bool ProjectContext::IsDebugMode()  const noexcept { return m_Config.debugMode; }

// ----------------------------------------------------------------------------
// Validation
// ----------------------------------------------------------------------------

bool ProjectContext::IsValid() const noexcept
{
    return !m_Config.repoRoot.empty()
        && !m_Config.projectId.empty()
        && !m_Config.version.empty();
}

// ----------------------------------------------------------------------------
// Path helpers
// ----------------------------------------------------------------------------

std::string ProjectContext::JoinPath(const std::string& base,
                                     const std::string& relative)
{
    if (relative.empty()) return base;

    // Treat already-absolute paths as-is (POSIX '/', Windows drive or UNC).
    bool isAbsolute = (relative[0] == '/')
                   || (relative[0] == '\\')
                   || (relative.size() >= 2
                       && std::isalpha(static_cast<unsigned char>(relative[0]))
                       && relative[1] == ':');
    if (isAbsolute) return relative;

    if (!base.empty() && (base.back() == '/' || base.back() == '\\'))
        return base + relative;

    return base + '/' + relative;
}

} // namespace NF::Game
