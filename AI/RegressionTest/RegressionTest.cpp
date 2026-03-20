#include "AI/RegressionTest/RegressionTest.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cstdio>

namespace AI {

RegressionTest& RegressionTest::Get() {
    static RegressionTest instance;
    return instance;
}

void RegressionTest::AddTest(const RegressionTestCase& tc) {
    m_tests.push_back(tc);
}

void RegressionTest::RemoveTest(const std::string& id) {
    m_tests.erase(std::remove_if(m_tests.begin(), m_tests.end(),
        [&](const RegressionTestCase& t){ return t.id == id; }), m_tests.end());
}

void RegressionTest::ClearSuite() { m_tests.clear(); }

void RegressionTest::OnTestComplete(ResultCallback cb) {
    m_onComplete = std::move(cb);
}

// ── AI test generation stub ────────────────────────────────────

std::vector<RegressionTestCase> RegressionTest::GenerateFromSource(const std::string& sourceFile) {
    std::vector<RegressionTestCase> generated;
    std::ifstream f(sourceFile);
    if (!f.is_open()) return generated;

    std::string line;
    uint32_t lineNo = 0;
    int testCounter = 0;
    while (std::getline(f, line)) {
        ++lineNo;
        // Heuristic: generate a test for every function definition
        if (line.find("void ") != std::string::npos ||
            line.find("bool ") != std::string::npos) {
            RegressionTestCase tc;
            tc.id          = sourceFile + "_line" + std::to_string(lineNo);
            tc.name        = "AutoTest_" + std::to_string(++testCounter);
            tc.description = "Auto-generated from " + sourceFile + ":" + std::to_string(lineNo);
            tc.targetFile  = sourceFile;
            tc.command     = "";   // to be filled by user/agent
            tc.enabled     = false;  // disabled until command is set
            generated.push_back(tc);
        }
    }
    return generated;
}

// ── Execution ──────────────────────────────────────────────────

RegressionSuiteResult RegressionTest::RunAll() {
    RegressionSuiteResult suite;
    suite.suiteId = "all";
    for (const auto& tc : m_tests) {
        auto result = Execute(tc);
        if (result.result == RegressionResult::Pass)    ++suite.passed;
        else if (result.result == RegressionResult::Fail) ++suite.failed;
        else ++suite.skipped;
        suite.totalSec += result.durationSec;
        suite.results.push_back(result);
    }
    m_history.push_back(suite);
    return suite;
}

RegressionTestResult RegressionTest::RunTest(const std::string& id) {
    for (const auto& tc : m_tests) {
        if (tc.id == id) return Execute(tc);
    }
    RegressionTestResult r;
    r.testId = id;
    r.result = RegressionResult::Skipped;
    r.failReason = "Test not found";
    return r;
}

RegressionTestResult RegressionTest::Execute(const RegressionTestCase& tc) {
    RegressionTestResult result;
    result.testId = tc.id;

    if (!tc.enabled || tc.command.empty()) {
        result.result = RegressionResult::Skipped;
        return result;
    }

    auto t0 = std::chrono::steady_clock::now();

    std::string cmd = tc.command + " 2>&1";
    std::string output;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buf[256];
        while (fgets(buf, sizeof(buf), pipe)) output += buf;
        int exitCode = pclose(pipe);
        result.output = output;

        bool pass = (exitCode == 0);
        for (const auto& expect : tc.expectedOutputContains) {
            if (output.find(expect) == std::string::npos) {
                pass = false;
                result.failReason += "Missing expected: " + expect + "; ";
            }
        }
        for (const auto& forbidden : tc.forbiddenOutput) {
            if (output.find(forbidden) != std::string::npos) {
                pass = false;
                result.failReason += "Forbidden output found: " + forbidden + "; ";
            }
        }
        result.result = pass ? RegressionResult::Pass : RegressionResult::Fail;
    } else {
        result.result    = RegressionResult::Fail;
        result.failReason = "Failed to execute command";
    }

    auto t1 = std::chrono::steady_clock::now();
    result.durationSec = std::chrono::duration<double>(t1 - t0).count();

    if (m_onComplete) m_onComplete(result);
    return result;
}

// ── Baseline ───────────────────────────────────────────────────

void RegressionTest::SaveBaseline(const std::string& baselineFile) {
    std::ofstream f(baselineFile);
    if (!f.is_open()) return;
    if (m_history.empty()) return;
    const auto& last = m_history.back();
    for (const auto& r : last.results) {
        f << r.testId << "|" << static_cast<int>(r.result) << "\n";
    }
}

bool RegressionTest::CompareToBaseline(const std::string& baselineFile,
                                        std::vector<std::string>& regressions) const {
    std::ifstream f(baselineFile);
    if (!f.is_open()) return false;
    std::unordered_map<std::string, RegressionResult> baseline;
    std::string line;
    while (std::getline(f, line)) {
        auto pos = line.find('|');
        if (pos == std::string::npos) continue;
        baseline[line.substr(0, pos)] =
            static_cast<RegressionResult>(std::stoi(line.substr(pos + 1)));
    }
    if (m_history.empty()) return true;
    const auto& last = m_history.back();
    for (const auto& r : last.results) {
        auto it = baseline.find(r.testId);
        if (it == baseline.end()) continue;
        if (it->second == RegressionResult::Pass && r.result != RegressionResult::Pass) {
            regressions.push_back(r.testId);
        }
    }
    return regressions.empty();
}

// ── JSON load/save stubs ───────────────────────────────────────

void RegressionTest::LoadSuiteFromJSON(const std::string& /*filePath*/) {
    // Full JSON parsing delegated to Core::Serializer in production
}

void RegressionTest::SaveSuiteToJSON(const std::string& /*filePath*/) const {
    // Full JSON serialization delegated to Core::Serializer in production
}

} // namespace AI
