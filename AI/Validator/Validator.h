#pragma once
// Validator — AI-driven code/build/PR validation reporter
//
// Runs a series of automated checks (build, tests, lint, diff analysis) and
// produces structured reports suitable for:
//   • Pull-request review summaries
//   • Pre-commit hooks
//   • Nightly automated validation pipelines
//
// The Validator is intentionally offline — it invokes local tools (cppcheck,
// cmake, ctest, git diff) and optionally feeds results to a local LLM for
// natural-language summaries.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace AI {

// ─────────────────────────────────────────────────────────────────────────────
// Check results
// ─────────────────────────────────────────────────────────────────────────────

enum class CheckStatus { Pass, Warn, Fail, Skip };

struct CheckResult {
    std::string  name;          // e.g. "Build (Debug)", "Tests", "cppcheck"
    CheckStatus  status       = CheckStatus::Skip;
    std::string  summary;       // one-line human-readable verdict
    std::string  rawOutput;     // full stdout/stderr
    int          exitCode     = 0;
    float        durationSecs = 0.f;
    std::vector<std::string> issues; // extracted error/warning lines
};

// ─────────────────────────────────────────────────────────────────────────────
// Diff analysis
// ─────────────────────────────────────────────────────────────────────────────

struct FileDiff {
    std::string path;
    int         linesAdded   = 0;
    int         linesRemoved = 0;
    bool        isBinary     = false;
    std::string language;   // inferred from extension
};

struct DiffSummary {
    std::string         fromRef;    // base commit / branch
    std::string         toRef;      // head commit / branch
    int                 totalAdded   = 0;
    int                 totalRemoved = 0;
    std::vector<FileDiff> files;
};

// ─────────────────────────────────────────────────────────────────────────────
// Validation report
// ─────────────────────────────────────────────────────────────────────────────

struct ValidationReport {
    std::string              title;
    std::string              timestamp;
    std::string              repoRoot;
    DiffSummary              diff;
    std::vector<CheckResult> checks;

    // AI-generated summary (optional, requires llmSummariseFn)
    std::string aiSummary;

    // Overall status — worst status across all checks
    CheckStatus OverallStatus() const;
    int         FailCount()     const;
    int         WarnCount()     const;
};

// ─────────────────────────────────────────────────────────────────────────────
// ValidatorConfig
// ─────────────────────────────────────────────────────────────────────────────

struct ValidatorConfig {
    std::string repoRoot;
    std::string buildDir;           // CMake build dir; created if absent
    std::string buildType = "Debug";
    std::string baseRef   = "HEAD~1"; // for diff computation

    bool runBuild    = true;
    bool runTests    = true;
    bool runLint     = true;   // cppcheck if available
    bool runDiffScan = true;   // parse git diff

    int  buildJobs   = 4;
    bool failFast    = false;  // stop after first failure

    // Where to write the Markdown report
    std::string reportPath = "Logs/Validation/report.md";

    // Optional LLM summariser: receives the full report text, returns summary
    std::function<std::string(const std::string&)> llmSummariseFn;
};

// ─────────────────────────────────────────────────────────────────────────────
// Validator
// ─────────────────────────────────────────────────────────────────────────────

class Validator {
public:
    explicit Validator(ValidatorConfig cfg);

    // ── Run pipeline ─────────────────────────────────────────────────────────

    /// Run all enabled checks. Returns the final report.
    ValidationReport Run();

    /// Run a single named check and return its result.
    CheckResult RunCheck(const std::string& name);

    // ── Individual checks ────────────────────────────────────────────────────

    CheckResult CheckBuild(const std::string& buildType = "Debug");
    CheckResult CheckTests();
    CheckResult CheckLint();
    CheckResult CheckDiff(const std::string& baseRef);

    // ── Reporting ─────────────────────────────────────────────────────────────

    /// Render report as Markdown.
    std::string RenderMarkdown(const ValidationReport& report) const;

    /// Write Markdown report to cfg.reportPath.
    bool WriteReport(const ValidationReport& report) const;

    // ── Callbacks ────────────────────────────────────────────────────────────

    using ProgressFn = std::function<void(const std::string& step, float pct)>;
    void SetProgressCallback(ProgressFn fn);

private:
    ValidatorConfig m_cfg;
    ProgressFn      m_onProgress;

    static std::string RunCommand(const std::string& cmd, int& exitCode,
                                   float& durationSecs);
    static std::vector<std::string> ExtractIssues(const std::string& output);
    static DiffSummary ParseGitDiff(const std::string& diffOutput,
                                     const std::string& fromRef,
                                     const std::string& toRef);
    void NotifyProgress(const std::string& step, float pct);
};

} // namespace AI
