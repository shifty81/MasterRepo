#pragma once
/**
 * @file IntegrationTestRunner.h
 * @brief Lightweight integration test runner for the NovaForge game loop.
 *
 * Provides a self-contained test harness that:
 *   - Boots the core game subsystems (Engine, ECS, Builder, PCG, SaveLoad)
 *   - Runs named test cases with pass/fail assertions
 *   - Collects results and prints a summary
 *   - Returns 0 on all-pass, 1 on any failure (usable as a CTest exit code)
 *
 * Tests exercise:
 *   - Core game loop tick (NF-02..NF-09)
 *   - Builder session: place part → weld → save → reload
 *   - PCG station generation
 *   - Save / load round-trip
 *   - Market economy tick
 *   - Faction reputation propagation
 *   - CombatSystem weapon arc + damage
 *   - ProgressionSystem XP / level-up
 *   - HazardSystem enter/exit events
 *   - NarrativeEngine template substitution + arc advancement
 *   - FleetController patrol tick
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Tests {

// ── Test result ───────────────────────────────────────────────────────────────

struct TestResult {
    std::string name;
    bool        passed{false};
    std::string failMessage;
};

// ── Assertion helpers ─────────────────────────────────────────────────────────

inline void Expect(bool condition, const std::string& msg,
                   std::string& failOut, bool& passed) {
    if (!condition) {
        passed  = false;
        failOut = msg;
    }
}

#define IT_EXPECT(cond, msg) \
    Expect((cond), (msg), _failMsg, _passed); \
    if (!_passed) return;

#define IT_EXPECT_EQ(a, b, msg) \
    Expect((a) == (b), (msg), _failMsg, _passed); \
    if (!_passed) return;

#define IT_EXPECT_GT(a, b, msg) \
    Expect((a) > (b), (msg), _failMsg, _passed); \
    if (!_passed) return;

// ── Test case type ────────────────────────────────────────────────────────────

using TestFn = std::function<void(bool& _passed, std::string& _failMsg)>;

struct TestCase {
    std::string name;
    TestFn      fn;
};

// ── IntegrationTestRunner ─────────────────────────────────────────────────────

class IntegrationTestRunner {
public:
    void AddTest(const std::string& name, TestFn fn);

    /// Run all registered tests. Returns number of failures.
    int Run();

    /// Print a formatted summary to stdout.
    void PrintSummary() const;

    const std::vector<TestResult>& Results() const;

private:
    std::vector<TestCase>   m_tests;
    std::vector<TestResult> m_results;
};

// ── Global test registration helper ──────────────────────────────────────────

IntegrationTestRunner& GlobalRunner();

} // namespace Tests
