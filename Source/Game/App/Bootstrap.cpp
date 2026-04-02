#include "Game/App/Bootstrap.h"
#include "Core/Config/ProjectManifest.h"
#include "Core/Logging/Log.h"
#include <memory>

namespace NF::Game {

// ----------------------------------------------------------------------------
// Internal state
// ----------------------------------------------------------------------------

struct Bootstrap::Impl {
    std::unique_ptr<ProjectContext> Context;
    std::unique_ptr<Session>        ActiveSession;
    ProjectManifest                 Manifest;
    bool                            Running = false;
};

// ----------------------------------------------------------------------------
// Construction
// ----------------------------------------------------------------------------

Bootstrap::Bootstrap()  : m_Impl(std::make_unique<Impl>()) {}
Bootstrap::~Bootstrap() { Shutdown(); }

// ----------------------------------------------------------------------------
// Run
// ----------------------------------------------------------------------------

BootstrapResult Bootstrap::Run(const BootstrapConfig& config)
{
    if (m_Impl->Running)
        return { false, "Bootstrap is already running" };

    Logger::Log(LogLevel::Info, "Game", "[1/4] Bootstrap — resolving repo root");

    // Step 1 — resolve repo root
    ProjectContextConfig ctxCfg = config.ContextConfig;
    if (ctxCfg.repoRoot.empty())
        ctxCfg.repoRoot = config.RepoRoot;

    if (ctxCfg.repoRoot.empty())
        return { false, "RepoRoot must not be empty" };

    // Step 2 — load project manifest
    Logger::Log(LogLevel::Info, "Game", "[2/4] Bootstrap — loading project manifest");
    {
        const std::string manifestPath =
            ctxCfg.repoRoot + "/Config/novaforge.project.json";

        if (m_Impl->Manifest.LoadFromFile(manifestPath))
        {
            m_Impl->Manifest.LogSummary();

            // Apply manifest paths into the project context config
            if (!m_Impl->Manifest.ContentRoot.empty())
                ctxCfg.contentRoot = m_Impl->Manifest.ContentRoot;
            if (!m_Impl->Manifest.ProjectVersion.empty())
                ctxCfg.version = m_Impl->Manifest.ProjectVersion;
            if (!m_Impl->Manifest.ProjectName.empty())
                ctxCfg.displayName = m_Impl->Manifest.ProjectName;
        }
        else
        {
            Logger::Log(LogLevel::Warning, "Game",
                        "Project manifest not found; using defaults");
        }
    }

    // Step 3 — build project context
    Logger::Log(LogLevel::Info, "Game", "[3/4] Bootstrap — building project context");
    m_Impl->Context = std::make_unique<ProjectContext>(ctxCfg);

    if (!m_Impl->Context->IsValid())
        return { false, "Project context validation failed" };

    Logger::Log(LogLevel::Info, "Game",
                "Project context: " + m_Impl->Context->DisplayName()
                + " v" + m_Impl->Context->Version());

    // Step 4 — create session
    Logger::Log(LogLevel::Info, "Game", "[4/4] Bootstrap — creating session");
    m_Impl->ActiveSession = std::make_unique<Session>();
    m_Impl->ActiveSession->OnConnected("local");

    m_Impl->Running = true;
    Logger::Log(LogLevel::Info, "Game", "Bootstrap complete");
    return { true, "Bootstrap complete" };
}

// ----------------------------------------------------------------------------
// Shutdown
// ----------------------------------------------------------------------------

void Bootstrap::Shutdown()
{
    if (!m_Impl->Running) return;

    if (m_Impl->ActiveSession && m_Impl->ActiveSession->IsConnected())
    {
        m_Impl->ActiveSession->OnDisconnecting();
        m_Impl->ActiveSession->OnDisconnected();
    }

    m_Impl->ActiveSession.reset();
    m_Impl->Context.reset();
    m_Impl->Running = false;
    Logger::Log(LogLevel::Info, "Game", "Bootstrap shutdown");
}

// ----------------------------------------------------------------------------
// Accessors
// ----------------------------------------------------------------------------

const ProjectContext* Bootstrap::GetProjectContext() const noexcept
{
    return m_Impl->Context.get();
}

const Session* Bootstrap::GetSession() const noexcept
{
    return m_Impl->ActiveSession.get();
}

bool Bootstrap::IsRunning() const noexcept { return m_Impl->Running; }

} // namespace NF::Game
