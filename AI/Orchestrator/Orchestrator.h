#pragma once
// Orchestrator — AI pipeline scheduler with priority and dependency management
//
// The Orchestrator manages a queue of AI "jobs" (code gen, PCG, test, audit,
// etc.) and dispatches them to the appropriate agents in dependency order.
// Jobs can be submitted programmatically or by the editor's NL interface.
//
// Key concepts:
//   • OrchestratorJob — unit of work with dependencies, priority, timeout
//   • JobGraph — DAG for resolving execution order
//   • AgentBinding — maps job types to agent implementations
//   • All execution is single-threaded by default; multi-threaded via
//     JobSystem if available

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace AI {

// ─────────────────────────────────────────────────────────────────────────────
// Job definition
// ─────────────────────────────────────────────────────────────────────────────

enum class JobStatus {
    Pending,
    Ready,       // all dependencies satisfied
    Running,
    Succeeded,
    Failed,
    Cancelled,
    TimedOut,
};

enum class JobPriority : uint8_t {
    Low     = 0,
    Normal  = 1,
    High    = 2,
    Urgent  = 3,
};

struct OrchestratorJob {
    std::string  id;
    std::string  type;            // "CodeGen", "PCGGen", "Audit", etc.
    std::string  description;
    std::string  agentType;       // which agent to dispatch to
    std::string  payload;         // JSON / free-form instruction
    JobPriority  priority    = JobPriority::Normal;
    float        timeoutSec  = 60.f;   // 0 = no timeout
    std::vector<std::string> dependsOn; // job IDs that must complete first

    // Result (set by agent)
    JobStatus   status      = JobStatus::Pending;
    std::string result;
    std::string errorMsg;
    float       durationSec = 0.f;
};

// ─────────────────────────────────────────────────────────────────────────────
// Agent binding
// ─────────────────────────────────────────────────────────────────────────────

using AgentExecuteFn = std::function<bool(OrchestratorJob&)>;

struct AgentBinding {
    std::string     agentType;
    AgentExecuteFn  execute;
    int             maxConcurrent = 1;
};

// ─────────────────────────────────────────────────────────────────────────────
// OrchestratorConfig
// ─────────────────────────────────────────────────────────────────────────────

struct OrchestratorConfig {
    int   maxConcurrentJobs = 4;
    bool  stopOnFailure     = false;
    float defaultTimeoutSec = 120.f;
    std::string logPath;
};

// ─────────────────────────────────────────────────────────────────────────────
// Orchestrator
// ─────────────────────────────────────────────────────────────────────────────

class Orchestrator {
public:
    explicit Orchestrator(OrchestratorConfig cfg = {});
    ~Orchestrator();

    // ── Agent registration ───────────────────────────────────────────────────

    void RegisterAgent(AgentBinding binding);
    void UnregisterAgent(const std::string& agentType);
    bool HasAgent(const std::string& agentType) const;

    // ── Job submission ───────────────────────────────────────────────────────

    std::string Submit(OrchestratorJob job);
    void        Cancel(const std::string& jobId);
    void        CancelAll();

    // ── Execution ────────────────────────────────────────────────────────────

    /// Process all ready jobs (up to maxConcurrentJobs). Call once per tick.
    int Tick();

    /// Block until all pending jobs finish or the timeout elapses.
    bool RunUntilComplete(float timeoutSec = 0.f);

    // ── Job queries ──────────────────────────────────────────────────────────

    const OrchestratorJob* GetJob(const std::string& id) const;
    std::vector<const OrchestratorJob*> GetJobsByStatus(JobStatus s) const;
    std::vector<const OrchestratorJob*> GetJobsByType(const std::string& type) const;

    int PendingCount()   const;
    int RunningCount()   const;
    int CompletedCount() const;
    int FailedCount()    const;

    // ── Callbacks ────────────────────────────────────────────────────────────

    using JobEventFn = std::function<void(const OrchestratorJob&)>;
    void OnJobStarted(JobEventFn fn);
    void OnJobFinished(JobEventFn fn);
    void OnJobFailed(JobEventFn fn);

    // ── Reporting ────────────────────────────────────────────────────────────

    std::string BuildReport() const;
    void        Clear();          // remove completed/failed jobs

private:
    OrchestratorConfig              m_cfg;
    std::vector<OrchestratorJob>    m_jobs;
    std::vector<AgentBinding>       m_agents;
    JobEventFn                      m_onStarted;
    JobEventFn                      m_onFinished;
    JobEventFn                      m_onFailed;

    std::string GenerateJobId() const;
    std::vector<OrchestratorJob*> BuildReadyList();
    bool DependenciesMet(const OrchestratorJob& job) const;
    AgentExecuteFn* FindAgent(const std::string& agentType);
};

} // namespace AI
