#include "AI/Orchestrator/Orchestrator.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace AI {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static uint64_t NowMs()
{
    return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

Orchestrator::Orchestrator(OrchestratorConfig cfg)
    : m_cfg(std::move(cfg))
{}

Orchestrator::~Orchestrator() {}

// ── Agent registration ───────────────────────────────────────────────────────

void Orchestrator::RegisterAgent(AgentBinding binding)
{
    auto it = std::find_if(m_agents.begin(), m_agents.end(),
                           [&](const AgentBinding& b){
                               return b.agentType == binding.agentType;
                           });
    if (it != m_agents.end()) *it = std::move(binding);
    else                      m_agents.push_back(std::move(binding));
}

void Orchestrator::UnregisterAgent(const std::string& agentType)
{
    m_agents.erase(
        std::remove_if(m_agents.begin(), m_agents.end(),
                       [&](const AgentBinding& b){
                           return b.agentType == agentType;
                       }),
        m_agents.end());
}

bool Orchestrator::HasAgent(const std::string& agentType) const
{
    return std::any_of(m_agents.begin(), m_agents.end(),
                       [&](const AgentBinding& b){
                           return b.agentType == agentType;
                       });
}

// ── Job submission ────────────────────────────────────────────────────────────

std::string Orchestrator::GenerateJobId() const
{
    return "job_" + std::to_string(NowMs()) + "_" +
           std::to_string(m_jobs.size());
}

std::string Orchestrator::Submit(OrchestratorJob job)
{
    if (job.id.empty()) job.id = GenerateJobId();
    if (job.timeoutSec <= 0.f) job.timeoutSec = m_cfg.defaultTimeoutSec;
    job.status = JobStatus::Pending;
    m_jobs.push_back(std::move(job));
    return m_jobs.back().id;
}

void Orchestrator::Cancel(const std::string& jobId)
{
    for (auto& j : m_jobs)
        if (j.id == jobId && j.status == JobStatus::Pending)
            j.status = JobStatus::Cancelled;
}

void Orchestrator::CancelAll()
{
    for (auto& j : m_jobs)
        if (j.status == JobStatus::Pending || j.status == JobStatus::Ready)
            j.status = JobStatus::Cancelled;
}

// ── Execution ─────────────────────────────────────────────────────────────────

bool Orchestrator::DependenciesMet(const OrchestratorJob& job) const
{
    for (const auto& dep : job.dependsOn) {
        bool found = false;
        for (const auto& j : m_jobs) {
            if (j.id == dep) {
                found = true;
                if (j.status != JobStatus::Succeeded) return false;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

std::vector<OrchestratorJob*> Orchestrator::BuildReadyList()
{
    std::vector<OrchestratorJob*> ready;
    for (auto& j : m_jobs) {
        if (j.status != JobStatus::Pending) continue;
        if (!DependenciesMet(j)) continue;
        j.status = JobStatus::Ready;
        ready.push_back(&j);
    }
    // Sort by priority descending
    std::sort(ready.begin(), ready.end(),
              [](const OrchestratorJob* a, const OrchestratorJob* b){
                  return (int)a->priority > (int)b->priority;
              });
    return ready;
}

AgentExecuteFn* Orchestrator::FindAgent(const std::string& agentType)
{
    for (auto& b : m_agents)
        if (b.agentType == agentType) return &b.execute;
    return nullptr;
}

int Orchestrator::Tick()
{
    auto ready = BuildReadyList();
    int dispatched = 0;

    for (auto* job : ready) {
        if (RunningCount() >= m_cfg.maxConcurrentJobs) break;
        if (job->status != JobStatus::Ready) continue;

        auto* execFn = FindAgent(job->agentType);
        if (!execFn) {
            job->status   = JobStatus::Failed;
            job->errorMsg = "No agent registered for type: " + job->agentType;
            if (m_onFailed) m_onFailed(*job);
            if (m_cfg.stopOnFailure) break;
            continue;
        }

        job->status = JobStatus::Running;
        if (m_onStarted) m_onStarted(*job);

        auto t0 = std::chrono::steady_clock::now();
        bool ok = (*execFn)(*job);
        job->durationSec = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - t0).count();

        if (ok) {
            job->status = JobStatus::Succeeded;
            if (m_onFinished) m_onFinished(*job);
        } else {
            job->status = JobStatus::Failed;
            if (m_onFailed) m_onFailed(*job);
            if (m_cfg.stopOnFailure) break;
        }
        ++dispatched;
    }
    return dispatched;
}

bool Orchestrator::RunUntilComplete(float timeoutSec)
{
    auto t0 = std::chrono::steady_clock::now();
    while (PendingCount() > 0 || RunningCount() > 0) {
        Tick();
        if (timeoutSec > 0.f) {
            float elapsed = std::chrono::duration<float>(
                std::chrono::steady_clock::now() - t0).count();
            if (elapsed >= timeoutSec) return false;
        }
    }
    return FailedCount() == 0;
}

// ── Queries ───────────────────────────────────────────────────────────────────

const OrchestratorJob* Orchestrator::GetJob(const std::string& id) const
{
    for (const auto& j : m_jobs)
        if (j.id == id) return &j;
    return nullptr;
}

std::vector<const OrchestratorJob*>
Orchestrator::GetJobsByStatus(JobStatus s) const
{
    std::vector<const OrchestratorJob*> out;
    for (const auto& j : m_jobs)
        if (j.status == s) out.push_back(&j);
    return out;
}

std::vector<const OrchestratorJob*>
Orchestrator::GetJobsByType(const std::string& type) const
{
    std::vector<const OrchestratorJob*> out;
    for (const auto& j : m_jobs)
        if (j.type == type) out.push_back(&j);
    return out;
}

int Orchestrator::PendingCount() const
{
    int n = 0;
    for (const auto& j : m_jobs)
        if (j.status == JobStatus::Pending || j.status == JobStatus::Ready) ++n;
    return n;
}

int Orchestrator::RunningCount() const
{
    int n = 0;
    for (const auto& j : m_jobs)
        if (j.status == JobStatus::Running) ++n;
    return n;
}

int Orchestrator::CompletedCount() const
{
    int n = 0;
    for (const auto& j : m_jobs)
        if (j.status == JobStatus::Succeeded) ++n;
    return n;
}

int Orchestrator::FailedCount() const
{
    int n = 0;
    for (const auto& j : m_jobs)
        if (j.status == JobStatus::Failed) ++n;
    return n;
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void Orchestrator::OnJobStarted(JobEventFn fn)  { m_onStarted  = std::move(fn); }
void Orchestrator::OnJobFinished(JobEventFn fn) { m_onFinished = std::move(fn); }
void Orchestrator::OnJobFailed(JobEventFn fn)   { m_onFailed   = std::move(fn); }

// ── Reporting ─────────────────────────────────────────────────────────────────

std::string Orchestrator::BuildReport() const
{
    std::ostringstream oss;
    oss << "# AI Orchestrator Report\n\n"
        << "| Status | Count |\n|--------|-------|\n"
        << "| Pending   | " << PendingCount()   << " |\n"
        << "| Running   | " << RunningCount()   << " |\n"
        << "| Succeeded | " << CompletedCount() << " |\n"
        << "| Failed    | " << FailedCount()    << " |\n\n"
        << "## Jobs\n\n"
        << "| ID | Type | Agent | Status | Duration |\n"
        << "|----|------|-------|--------|----------|\n";
    for (const auto& j : m_jobs) {
        const char* st =
            (j.status == JobStatus::Succeeded) ? "✅" :
            (j.status == JobStatus::Failed)    ? "❌" :
            (j.status == JobStatus::Running)   ? "⏳" :
            (j.status == JobStatus::Cancelled) ? "🚫" : "⌛";
        oss << "| " << j.id << " | " << j.type << " | "
            << j.agentType << " | " << st << " | "
            << j.durationSec << "s |\n";
    }
    return oss.str();
}

void Orchestrator::Clear()
{
    m_jobs.erase(
        std::remove_if(m_jobs.begin(), m_jobs.end(),
                       [](const OrchestratorJob& j){
                           return j.status == JobStatus::Succeeded ||
                                  j.status == JobStatus::Failed    ||
                                  j.status == JobStatus::Cancelled;
                       }),
        m_jobs.end());
}

} // namespace AI
