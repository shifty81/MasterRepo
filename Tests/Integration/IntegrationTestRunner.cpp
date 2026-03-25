#include "Tests/Integration/IntegrationTestRunner.h"
#include <iostream>

namespace Tests {

void IntegrationTestRunner::AddTest(const std::string& name, TestFn fn) {
    m_tests.push_back({name, std::move(fn)});
}

int IntegrationTestRunner::Run() {
    m_results.clear();
    int failures = 0;
    for (const auto& tc : m_tests) {
        bool   passed   = true;
        std::string msg = "";
        try {
            tc.fn(passed, msg);
        } catch (const std::exception& e) {
            passed = false;
            msg    = std::string("exception: ") + e.what();
        } catch (...) {
            passed = false;
            msg    = "unknown exception";
        }
        m_results.push_back({tc.name, passed, msg});
        if (!passed) ++failures;
    }
    return failures;
}

void IntegrationTestRunner::PrintSummary() const {
    int pass = 0, fail = 0;
    for (const auto& r : m_results) {
        if (r.passed) {
            ++pass;
            std::cout << "  [PASS] " << r.name << "\n";
        } else {
            ++fail;
            std::cout << "  [FAIL] " << r.name << " — " << r.failMessage << "\n";
        }
    }
    std::cout << "\n--- " << (pass + fail) << " tests: "
              << pass << " passed, " << fail << " failed ---\n";
}

const std::vector<TestResult>& IntegrationTestRunner::Results() const {
    return m_results;
}

IntegrationTestRunner& GlobalRunner() {
    static IntegrationTestRunner instance;
    return instance;
}

} // namespace Tests
