#pragma once
// TestPipeline — Automated testing pipeline (Blueprint: dev_tools/automated_testing_pipeline)
//
// Orchestrates the full test lifecycle:
//   1. AI test generation (TestGen) — produce test cases from code/specs
//   2. Test compilation — build generated tests via cmake
//   3. Test execution — run via ctest / custom runner
//   4. Regression comparison — compare against previous run baselines
//   5. Report generation — Markdown + JSON
//
// Integrates with: AI::TestGen, AI::RegressionTest, AI::Validator,
//                  Tools::AdminConsole (for server-side test runs).

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Tools {

// ─────────────────────────────────────────────────────────────────────────────
// Individual test case
// ─────────────────────────────────────────────────────────────────────────────

enum class TestOutcome { Pass, Fail, Skip, Timeout, Error };

struct TestCase {
    std::string  id;
    std::string  name;
    std::string  suiteName;
    std::string  sourceCode;   // generated or loaded C++/Python
    std::string  targetModule; // which module this tests
    bool         isGenerated = false; // true = produced by TestGen
};

struct TestResult {
    TestCase     testCase;
    TestOutcome  outcome     = TestOutcome::Skip;
    std::string  output;      // stdout/stderr
    float        durationSec = 0.f;
    std::string  errorMsg;
    bool         isRegression = false; // true = was passing before
};

// ─────────────────────────────────────────────────────────────────────────────
// Test suite
// ─────────────────────────────────────────────────────────────────────────────

struct TestSuite {
    std::string              name;
    std::vector<TestCase>    tests;
    std::string              buildTarget; // cmake target
};

// ─────────────────────────────────────────────────────────────────────────────
// Pipeline run result
// ─────────────────────────────────────────────────────────────────────────────

struct PipelineRunResult {
    bool                      passed       = true;
    std::vector<TestResult>   results;
    int                       totalTests   = 0;
    int                       passCount    = 0;
    int                       failCount    = 0;
    int                       skipCount    = 0;
    int                       regressionCount = 0;
    float                     totalDurSec  = 0.f;
    std::string               timestamp;
    std::string               baselineRef;  // git ref of baseline
};

// ─────────────────────────────────────────────────────────────────────────────
// TestPipelineConfig
// ─────────────────────────────────────────────────────────────────────────────

struct TestPipelineConfig {
    std::string repoRoot;
    std::string buildDir;
    std::string buildType       = "Debug";
    int         buildJobs       = 4;
    bool        generateTests   = true;  // run TestGen first
    bool        runRegression   = true;  // compare to baseline
    std::string baselineRef     = "HEAD~1";
    std::string reportPath      = "Logs/TestPipeline/report.md";
    float       testTimeoutSec  = 30.f;
    bool        failFast        = false;
    // Optional: test generation hook (called per module)
    std::function<std::vector<TestCase>(const std::string& module)> testGenFn;
};

// ─────────────────────────────────────────────────────────────────────────────
// Baseline store (compare current run to previous)
// ─────────────────────────────────────────────────────────────────────────────

class BaselineStore {
public:
    explicit BaselineStore(const std::string& path = "");

    bool Load(const std::string& ref);
    bool Save(const PipelineRunResult& result) const;

    bool WasPassing(const std::string& testId) const;

    size_t BaselineCount() const;

private:
    std::string m_path;
    std::unordered_map<std::string, bool> m_baseline;
};

// ─────────────────────────────────────────────────────────────────────────────
// TestPipeline
// ─────────────────────────────────────────────────────────────────────────────

class TestPipeline {
public:
    explicit TestPipeline(TestPipelineConfig cfg = {});
    ~TestPipeline();

    // ── Run ──────────────────────────────────────────────────────────────────

    PipelineRunResult Run();
    PipelineRunResult RunSuite(const TestSuite& suite);

    // ── Stage control ────────────────────────────────────────────────────────

    std::vector<TestCase> GenerateTests(const std::string& module);
    bool                  BuildTests(const std::string& buildTarget);
    std::vector<TestResult> ExecuteTests(const std::vector<TestCase>& tests);
    std::vector<TestResult> CompareToBaseline(
                              const std::vector<TestResult>& results);

    // ── Reporting ────────────────────────────────────────────────────────────

    std::string BuildMarkdownReport(const PipelineRunResult& result) const;
    bool        WriteReport(const PipelineRunResult& result) const;

    // ── Callbacks ────────────────────────────────────────────────────────────

    using ProgressFn = std::function<void(const std::string& step, float pct)>;
    void SetProgressCallback(ProgressFn fn);

    using ResultFn = std::function<void(const TestResult&)>;
    void SetTestResultCallback(ResultFn fn);

private:
    TestPipelineConfig  m_cfg;
    ProgressFn          m_onProgress;
    ResultFn            m_onResult;

    void Notify(const std::string& step, float pct);

    static std::string RunCommand(const std::string& cmd, int& exitCode,
                                   float& durationSec);
};

} // namespace Tools
