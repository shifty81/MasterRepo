#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace AI {

// ──────────────────────────────────────────────────────────────
// Regression test case
// ──────────────────────────────────────────────────────────────

enum class RegressionResult { Pass, Fail, Skipped, Timeout };

struct RegressionTestCase {
    std::string      id;
    std::string      name;
    std::string      description;
    std::string      targetFile;     // source file under test
    std::string      command;        // shell command to execute the test
    double           timeoutSec = 30.0;
    std::vector<std::string> expectedOutputContains;
    std::vector<std::string> forbiddenOutput;
    bool             enabled = true;
};

struct RegressionTestResult {
    std::string      testId;
    RegressionResult result  = RegressionResult::Skipped;
    std::string      output;
    double           durationSec = 0.0;
    std::string      failReason;
};

struct RegressionSuiteResult {
    std::string                        suiteId;
    std::vector<RegressionTestResult>  results;
    uint32_t passed   = 0;
    uint32_t failed   = 0;
    uint32_t skipped  = 0;
    double   totalSec = 0.0;
    bool allPassed() const { return failed == 0; }
};

// ──────────────────────────────────────────────────────────────
// RegressionTest — automated AI-generated regression test runner
// ──────────────────────────────────────────────────────────────

class RegressionTest {
public:
    // Suite management
    void AddTest(const RegressionTestCase& tc);
    void RemoveTest(const std::string& id);
    void LoadSuiteFromJSON(const std::string& filePath);
    void SaveSuiteToJSON(const std::string& filePath) const;
    void ClearSuite();

    // AI test generation: inspects source and generates test stubs
    std::vector<RegressionTestCase> GenerateFromSource(const std::string& sourceFile);

    // Execution
    RegressionSuiteResult RunAll();
    RegressionTestResult  RunTest(const std::string& id);

    // Baseline / snapshot comparison
    void   SaveBaseline(const std::string& baselineFile);
    bool   CompareToBaseline(const std::string& baselineFile,
                             std::vector<std::string>& regressions) const;

    // Callbacks
    using ResultCallback = std::function<void(const RegressionTestResult&)>;
    void OnTestComplete(ResultCallback cb);

    // Queries
    size_t TestCount() const { return m_tests.size(); }
    const std::vector<RegressionTestCase>& Tests() const { return m_tests; }

    // Singleton
    static RegressionTest& Get();

private:
    RegressionTestResult Execute(const RegressionTestCase& tc);

    std::vector<RegressionTestCase>    m_tests;
    std::vector<RegressionSuiteResult> m_history;
    ResultCallback                     m_onComplete;
};

} // namespace AI
