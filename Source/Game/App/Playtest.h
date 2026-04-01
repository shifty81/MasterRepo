#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace NF::Game {

// ============================================================================
// PlaytestSession
// ============================================================================

/// @brief Configuration for one automated playtest run.
struct PlaytestConfig {
    int         TickCount          = 60;           ///< Number of simulation ticks to execute.
    float       DeltaTime          = 1.0f / 60.0f; ///< Fixed delta time per tick (seconds).
    bool        LoadSave           = false;         ///< Attempt to load @c SaveName before ticking.
    std::string SaveName;                           ///< Save slot name (used when LoadSave is true).
    std::string ScenarioTag;                        ///< Optional label for log output.
    bool        AbortOnFirstError  = true;          ///< Stop the tick loop on the first error.
};

/// @brief Result produced by PlaytestSession::Run().
struct PlaytestResult {
    bool        Success    = false;
    int         TicksRun   = 0;
    int         ErrorCount = 0;
    std::vector<std::string> Errors;
    std::vector<std::string> Log;

    /// @brief Returns 0 on success, 1 on failure (suitable as process exit code).
    [[nodiscard]] int ExitCode() const noexcept { return Success ? 0 : 1; }

    /// @brief Formats the result as a multi-line human-readable summary.
    [[nodiscard]] std::string Format() const;
};

/// @brief Callback invoked once per tick; return false to abort the run early.
using PlaytestTickCallback = std::function<bool(int tickIndex, float dt)>;

/// @brief Runs the game in headless mode for a fixed number of ticks.
///
/// Suitable for CI smoke tests and automated validation.
class PlaytestSession {
public:
    /// @brief Construct with the given configuration (defaults to 60 ticks).
    explicit PlaytestSession(const PlaytestConfig& config = {});

    /// @brief Set an optional per-tick callback for custom assertions.
    void SetTickCallback(PlaytestTickCallback cb) { m_TickCallback = std::move(cb); }

    /// @brief Execute the full session.  Call only once per instance.
    [[nodiscard]] PlaytestResult Run();

private:
    PlaytestConfig       m_Config;
    PlaytestTickCallback m_TickCallback;

    void EmitLog  (PlaytestResult& result, const std::string& msg) const;
    void EmitError(PlaytestResult& result, const std::string& msg);
};

// ============================================================================
// TestHarness
// ============================================================================

/// @brief Aggregate result from TestHarness::RunAll().
struct HarnessResult {
    int  TotalCases  = 0;
    int  PassedCases = 0;
    int  FailedCases = 0;
    std::vector<std::string>    FailedNames;
    std::vector<PlaytestResult> CaseResults;

    /// @brief Returns true when all cases passed.
    [[nodiscard]] bool AllPassed() const noexcept { return FailedCases == 0; }

    /// @brief Formats the result as a multi-line human-readable summary.
    [[nodiscard]] std::string Format() const;
};

/// @brief Describes one named playtest case inside a TestHarness.
struct TestCase {
    std::string    Name;
    PlaytestConfig SessionConfig;
    std::function<void()> Setup;    ///< Called before the session runs (optional).
    std::function<void()> Teardown; ///< Called after the session runs (optional).
};

/// @brief Runs a collection of @c PlaytestSession cases and reports results.
class TestHarness {
public:
    /// @brief Add a pre-configured test case.
    void AddCase(TestCase tc);

    /// @brief Add a case by name with a tick count and optional scenario tag.
    void AddCase(const std::string& name,
                 int                tickCount   = 60,
                 const std::string& scenarioTag = {});

    /// @brief Run all registered cases in order and return the aggregate result.
    [[nodiscard]] HarnessResult RunAll();

private:
    std::vector<TestCase> m_Cases;
};

} // namespace NF::Game
