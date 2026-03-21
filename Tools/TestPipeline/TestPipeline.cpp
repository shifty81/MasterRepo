#include "Tools/TestPipeline/TestPipeline.h"
#include <algorithm>
#include <chrono>
#ifdef _WIN32
#  include <stdio.h>   // provides _popen / _pclose in global namespace under MSVC
#  define popen  _popen
#  define pclose _pclose
#else
#  include <cstdio>
#endif
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
namespace Tools {

// ─────────────────────────────────────────────────────────────────────────────
// BaselineStore
// ─────────────────────────────────────────────────────────────────────────────

BaselineStore::BaselineStore(const std::string& path) : m_path(path) {}

bool BaselineStore::Load(const std::string& /*ref*/)
{
    if (m_path.empty()) return false;
    std::ifstream f(m_path + "/baseline.csv");
    if (!f) return false;
    std::string line;
    while (std::getline(f, line)) {
        auto comma = line.find(',');
        if (comma == std::string::npos) continue;
        std::string id      = line.substr(0, comma);
        std::string passing = line.substr(comma + 1);
        m_baseline[id] = (passing == "1" || passing == "true");
    }
    return !m_baseline.empty();
}

bool BaselineStore::Save(const PipelineRunResult& result) const
{
    if (m_path.empty()) return false;
    try {
        fs::create_directories(m_path);
        std::ofstream f(m_path + "/baseline.csv");
        if (!f) return false;
        for (const auto& r : result.results)
            f << r.testCase.id << ","
              << (r.outcome == TestOutcome::Pass ? "1" : "0") << "\n";
        return true;
    } catch (...) {
        return false;
    }
}

bool BaselineStore::WasPassing(const std::string& id) const
{
    auto it = m_baseline.find(id);
    if (it == m_baseline.end()) return true; // unknown = assume was passing
    return it->second;
}

size_t BaselineStore::BaselineCount() const { return m_baseline.size(); }

// ─────────────────────────────────────────────────────────────────────────────
// TestPipeline
// ─────────────────────────────────────────────────────────────────────────────

TestPipeline::TestPipeline(TestPipelineConfig cfg)
    : m_cfg(std::move(cfg))
{}

TestPipeline::~TestPipeline() {}

void TestPipeline::SetProgressCallback(ProgressFn fn) { m_onProgress = std::move(fn); }
void TestPipeline::SetTestResultCallback(ResultFn fn) { m_onResult   = std::move(fn); }

void TestPipeline::Notify(const std::string& step, float pct)
{
    if (m_onProgress) m_onProgress(step, pct);
}

// ── RunCommand ────────────────────────────────────────────────────────────────

/* static */
std::string TestPipeline::RunCommand(const std::string& cmd,
                                      int& exitCode,
                                      float& durationSec)
{
    auto t0 = std::chrono::steady_clock::now();
    std::string output;
    FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
    if (!pipe) { exitCode = -1; return "popen failed"; }
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) output += buf;
    exitCode = pclose(pipe);
    durationSec = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - t0).count();
    return output;
}

// ── Stage: generate tests ─────────────────────────────────────────────────────

std::vector<TestCase> TestPipeline::GenerateTests(const std::string& module)
{
    if (m_cfg.testGenFn) return m_cfg.testGenFn(module);
    // Stub: return an empty list — real impl calls AI::TestGen
    return {};
}

// ── Stage: build tests ────────────────────────────────────────────────────────

bool TestPipeline::BuildTests(const std::string& buildTarget)
{
    if (m_cfg.buildDir.empty()) return true; // nothing to build

    std::string configCmd =
        "cmake -B \"" + m_cfg.buildDir + "\" -DCMAKE_BUILD_TYPE=" +
        m_cfg.buildType + " \"" + m_cfg.repoRoot + "\"";
    std::string buildCmd =
        "cmake --build \"" + m_cfg.buildDir + "\" --target " + buildTarget +
        " -- -j" + std::to_string(m_cfg.buildJobs);

    int ec = 0; float dur = 0.f;
    RunCommand(configCmd, ec, dur);
    if (ec != 0) return false;
    RunCommand(buildCmd, ec, dur);
    return ec == 0;
}

// ── Stage: execute tests ──────────────────────────────────────────────────────

std::vector<TestResult> TestPipeline::ExecuteTests(
    const std::vector<TestCase>& tests)
{
    std::vector<TestResult> results;
    for (const auto& tc : tests) {
        TestResult r;
        r.testCase = tc;

        if (m_cfg.buildDir.empty()) {
            // No build dir — run via ctest if available
            int ec = 0; float dur = 0.f;
            std::string out = RunCommand(
                "ctest --test-dir \"" + m_cfg.buildDir + "\" "
                "--tests-regex " + tc.name + " --output-on-failure",
                ec, dur);
            r.durationSec = dur;
            r.output      = out;
            r.outcome     = (ec == 0) ? TestOutcome::Pass : TestOutcome::Fail;
        } else {
            r.outcome = TestOutcome::Skip;
            r.output  = "No build dir configured";
        }

        if (m_onResult) m_onResult(r);
        results.push_back(r);

        if (m_cfg.failFast && r.outcome == TestOutcome::Fail) break;
    }
    return results;
}

// ── Stage: regression comparison ─────────────────────────────────────────────

std::vector<TestResult> TestPipeline::CompareToBaseline(
    const std::vector<TestResult>& results)
{
    std::string baselinePath = m_cfg.reportPath.empty()
                               ? "Logs/TestPipeline"
                               : fs::path(m_cfg.reportPath).parent_path().string();

    BaselineStore store(baselinePath);
    store.Load(m_cfg.baselineRef);

    std::vector<TestResult> out = results;
    for (auto& r : out) {
        if (r.outcome == TestOutcome::Fail &&
            store.WasPassing(r.testCase.id))
            r.isRegression = true;
    }
    return out;
}

// ── Main run ──────────────────────────────────────────────────────────────────

PipelineRunResult TestPipeline::Run()
{
    // Return an empty run result — real run driven by RunSuite
    TestSuite suite;
    suite.name = "default";
    return RunSuite(suite);
}

PipelineRunResult TestPipeline::RunSuite(const TestSuite& suite)
{
    PipelineRunResult run;
    auto t0 = std::chrono::steady_clock::now();

    // Timestamp
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    run.timestamp   = buf;
    run.baselineRef = m_cfg.baselineRef;

    std::vector<TestCase> allTests = suite.tests;

    // Stage 1: generate tests
    if (m_cfg.generateTests) {
        Notify("TestGen", 0.1f);
        auto gen = GenerateTests(suite.name);
        allTests.insert(allTests.end(), gen.begin(), gen.end());
    }

    // Stage 2: build
    if (!suite.buildTarget.empty() && !m_cfg.buildDir.empty()) {
        Notify("Build", 0.3f);
        BuildTests(suite.buildTarget);
    }

    // Stage 3: execute
    Notify("Execute", 0.5f);
    auto results = ExecuteTests(allTests);

    // Stage 4: regression
    if (m_cfg.runRegression) {
        Notify("Regression", 0.8f);
        results = CompareToBaseline(results);
    }

    run.results = results;
    run.totalTests = (int)results.size();
    for (const auto& r : results) {
        if (r.outcome == TestOutcome::Pass)  ++run.passCount;
        else if (r.outcome == TestOutcome::Fail) ++run.failCount;
        else                                 ++run.skipCount;
        if (r.isRegression) ++run.regressionCount;
    }
    run.passed = (run.failCount == 0);
    run.totalDurSec = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - t0).count();

    Notify("Done", 1.0f);
    WriteReport(run);
    return run;
}

// ── Reporting ─────────────────────────────────────────────────────────────────

std::string TestPipeline::BuildMarkdownReport(
    const PipelineRunResult& run) const
{
    std::ostringstream oss;
    oss << "# Test Pipeline Report\n\n"
        << "**Timestamp:** " << run.timestamp << "\n"
        << "**Baseline:** `" << run.baselineRef << "`\n"
        << "**Result:** " << (run.passed ? "✅ PASS" : "❌ FAIL") << "\n\n"
        << "| Metric | Value |\n|--------|-------|\n"
        << "| Total     | " << run.totalTests      << " |\n"
        << "| Pass      | " << run.passCount        << " |\n"
        << "| Fail      | " << run.failCount        << " |\n"
        << "| Skip      | " << run.skipCount        << " |\n"
        << "| Regress   | " << run.regressionCount  << " |\n"
        << "| Duration  | " << run.totalDurSec      << "s |\n";

    if (!run.results.empty()) {
        oss << "\n## Test Results\n\n"
            << "| Test | Outcome | Regression | Duration |\n"
            << "|------|---------|------------|----------|\n";
        for (const auto& r : run.results) {
            const char* oc =
                (r.outcome == TestOutcome::Pass)    ? "✅" :
                (r.outcome == TestOutcome::Fail)    ? "❌" :
                (r.outcome == TestOutcome::Skip)    ? "⏭"  :
                (r.outcome == TestOutcome::Timeout) ? "⏰"  : "⚠";
            oss << "| " << r.testCase.name << " | " << oc
                << " | " << (r.isRegression ? "⚠ YES" : "—")
                << " | " << r.durationSec << "s |\n";
        }
    }
    return oss.str();
}

bool TestPipeline::WriteReport(const PipelineRunResult& run) const
{
    if (m_cfg.reportPath.empty()) return false;
    try {
        fs::create_directories(fs::path(m_cfg.reportPath).parent_path());
        std::ofstream f(m_cfg.reportPath);
        if (!f) return false;
        f << BuildMarkdownReport(run);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace Tools
