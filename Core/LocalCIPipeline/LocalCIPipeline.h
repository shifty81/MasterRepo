#pragma once
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <unordered_map>

namespace Core {

// ──────────────────────────────────────────────────────────────
// Build/test step result
// ──────────────────────────────────────────────────────────────

enum class CIStepStatus { Pending, Running, Passed, Failed, Skipped };

struct CIStepResult {
    std::string    name;
    CIStepStatus   status    = CIStepStatus::Pending;
    std::string    output;
    int            exitCode  = 0;
    double         durationMs = 0.0;
};

// ──────────────────────────────────────────────────────────────
// CI pipeline run result
// ──────────────────────────────────────────────────────────────

struct CIRunResult {
    std::string              id;          // unique run ID
    std::string              trigger;     // e.g. "file_save", "manual", "git_commit"
    std::vector<CIStepResult> steps;
    bool                     passed     = false;
    double                   totalMs    = 0.0;
    std::string              timestamp;
};

// ──────────────────────────────────────────────────────────────
// LocalCIPipeline — automated build/test on file save or trigger
// ──────────────────────────────────────────────────────────────

class LocalCIPipeline {
public:
    using RunCallback = std::function<void(const CIRunResult&)>;

    // Configuration
    void SetBuildDir(const std::string& dir);
    void SetSourceDir(const std::string& dir);
    void SetBuildCommand(const std::string& cmd);
    void SetTestCommand(const std::string& cmd);

    // Add custom steps (name, shell command)
    void AddStep(const std::string& name, const std::string& command);
    void ClearSteps();

    // Watch a directory/file for changes and auto-trigger
    void WatchPath(const std::string& path);
    void StopWatching();

    // Manual trigger
    CIRunResult RunNow(const std::string& trigger = "manual");

    // Register callback invoked after each run
    void OnRunComplete(RunCallback cb);

    // Run history
    const std::vector<CIRunResult>& History() const { return m_history; }
    void ClearHistory();

    // Status
    bool IsRunning() const { return m_running; }
    size_t TotalRuns()  const { return m_history.size(); }
    size_t PassedRuns() const;
    size_t FailedRuns() const;

private:
    struct Step {
        std::string name;
        std::string command;
    };

    CIStepResult  ExecuteStep(const Step& step);
    std::string   GenerateRunId() const;
    std::string   CurrentTimestamp() const;

    std::string           m_buildDir;
    std::string           m_sourceDir;
    std::string           m_buildCmd  = "cmake --build .";
    std::string           m_testCmd   = "ctest --output-on-failure";
    std::vector<Step>     m_steps;
    std::vector<std::string> m_watchPaths;
    std::vector<CIRunResult> m_history;
    RunCallback           m_onComplete;
    bool                  m_running   = false;
    bool                  m_watching  = false;
};

} // namespace Core
