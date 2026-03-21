#include "Core/LocalCIPipeline/LocalCIPipeline.h"
#include <sstream>
#include <cstdio>
#ifdef _WIN32
#  define popen  _popen
#  define pclose _pclose
#endif
#include <ctime>
#include <random>
#include <algorithm>

namespace Core {

// ── configuration ──────────────────────────────────────────────

void LocalCIPipeline::SetBuildDir(const std::string& dir)    { m_buildDir = dir; }
void LocalCIPipeline::SetSourceDir(const std::string& dir)   { m_sourceDir = dir; }
void LocalCIPipeline::SetBuildCommand(const std::string& cmd) { m_buildCmd = cmd; }
void LocalCIPipeline::SetTestCommand(const std::string& cmd)  { m_testCmd  = cmd; }

void LocalCIPipeline::AddStep(const std::string& name, const std::string& cmd) {
    m_steps.push_back({name, cmd});
}
void LocalCIPipeline::ClearSteps() { m_steps.clear(); }

void LocalCIPipeline::WatchPath(const std::string& path) {
    m_watchPaths.push_back(path);
    m_watching = true;
}
void LocalCIPipeline::StopWatching() {
    m_watchPaths.clear();
    m_watching = false;
}

void LocalCIPipeline::OnRunComplete(RunCallback cb) { m_onComplete = std::move(cb); }
void LocalCIPipeline::ClearHistory() { m_history.clear(); }

// ── statistics ─────────────────────────────────────────────────

size_t LocalCIPipeline::PassedRuns() const {
    return static_cast<size_t>(
        std::count_if(m_history.begin(), m_history.end(),
                      [](const CIRunResult& r){ return r.passed; }));
}
size_t LocalCIPipeline::FailedRuns() const {
    return TotalRuns() - PassedRuns();
}

// ── run ────────────────────────────────────────────────────────

CIRunResult LocalCIPipeline::RunNow(const std::string& trigger) {
    m_running = true;

    CIRunResult run;
    run.id        = GenerateRunId();
    run.trigger   = trigger;
    run.timestamp = CurrentTimestamp();
    run.passed    = true;

    // Build default step list if empty
    std::vector<Step> steps = m_steps;
    if (steps.empty()) {
        if (!m_buildCmd.empty()) steps.push_back({"build", m_buildCmd});
        if (!m_testCmd.empty())  steps.push_back({"test",  m_testCmd});
    }

    auto wallStart = std::chrono::steady_clock::now();

    for (const auto& step : steps) {
        CIStepResult sr = ExecuteStep(step);
        if (sr.status == CIStepStatus::Failed) run.passed = false;
        run.steps.push_back(std::move(sr));
        if (!run.passed) break; // stop on first failure
    }

    auto wallEnd = std::chrono::steady_clock::now();
    run.totalMs = std::chrono::duration<double, std::milli>(wallEnd - wallStart).count();

    m_history.push_back(run);
    m_running = false;

    if (m_onComplete) m_onComplete(run);
    return run;
}

// ── private helpers ────────────────────────────────────────────

CIStepResult LocalCIPipeline::ExecuteStep(const Step& step) {
    CIStepResult result;
    result.name   = step.name;
    result.status = CIStepStatus::Running;

    std::string cmd = step.command;
    if (!m_buildDir.empty()) {
        // Run command inside build dir if specified
        cmd = "cd \"" + m_buildDir + "\" && " + cmd;
    }
    cmd += " 2>&1";   // merge stderr

    auto t0 = std::chrono::steady_clock::now();

    std::string output;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buf[256];
        while (fgets(buf, sizeof(buf), pipe)) {
            output += buf;
        }
        result.exitCode = pclose(pipe);
    } else {
        result.exitCode = -1;
        output = "Failed to open pipe for command: " + step.command;
    }

    auto t1 = std::chrono::steady_clock::now();
    result.durationMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    result.output     = std::move(output);
    result.status     = (result.exitCode == 0) ? CIStepStatus::Passed : CIStepStatus::Failed;
    return result;
}

std::string LocalCIPipeline::GenerateRunId() const {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<uint32_t> dist;
    std::ostringstream oss;
    oss << "ci-" << dist(rng);
    return oss.str();
}

std::string LocalCIPipeline::CurrentTimestamp() const {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
    return buf;
}

} // namespace Core
