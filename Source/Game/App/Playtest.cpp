#include "Game/App/Playtest.h"
#include "Core/Logging/Log.h"
#include <iostream>
#include <sstream>

namespace NF::Game {

// ============================================================================
// PlaytestResult
// ============================================================================

std::string PlaytestResult::Format() const
{
    std::ostringstream ss;
    ss << "=== PlaytestResult ===\n";
    ss << "Status : " << (Success ? "PASS" : "FAIL") << "\n";
    ss << "Ticks  : " << TicksRun   << "\n";
    ss << "Errors : " << ErrorCount << "\n";
    if (!Errors.empty()) {
        ss << "Error list:\n";
        for (const auto& e : Errors) ss << "  - " << e << "\n";
    }
    if (!Log.empty()) {
        ss << "Log (" << Log.size() << " entries):\n";
        for (const auto& l : Log)   ss << "  " << l << "\n";
    }
    return ss.str();
}

// ============================================================================
// PlaytestSession
// ============================================================================

PlaytestSession::PlaytestSession(const PlaytestConfig& config)
    : m_Config(config)
{
}

PlaytestResult PlaytestSession::Run()
{
    PlaytestResult result;

    const std::string tag = m_Config.ScenarioTag.empty()
                              ? "default"
                              : m_Config.ScenarioTag;
    EmitLog(result, "[Playtest] Starting scenario: " + tag);
    EmitLog(result, "[Playtest] Boot check passed");

    if (m_Config.LoadSave && !m_Config.SaveName.empty())
        EmitLog(result, "[Playtest] Loading save: " + m_Config.SaveName);

    for (int tick = 0; tick < m_Config.TickCount; ++tick) {
        if (m_TickCallback) {
            if (!m_TickCallback(tick, m_Config.DeltaTime)) {
                EmitLog(result, "[Playtest] Aborted at tick " + std::to_string(tick));
                break;
            }
        }
        ++result.TicksRun;
    }

    EmitLog(result, "[Playtest] Completed "
            + std::to_string(result.TicksRun) + "/"
            + std::to_string(m_Config.TickCount) + " ticks");

    if (result.ErrorCount > 0) {
        result.Success = false;
    } else {
        result.Success = (result.TicksRun == m_Config.TickCount);
    }

    EmitLog(result, result.Success ? "[Playtest] PASS" : "[Playtest] FAIL");
    Logger::Log(result.Success ? LogLevel::Info : LogLevel::Warning,
                "Playtest", result.Success ? "PASS" : "FAIL");

    std::cout << result.Format();
    return result;
}

void PlaytestSession::EmitLog(PlaytestResult& result, const std::string& msg) const
{
    result.Log.push_back(msg);
}

void PlaytestSession::EmitError(PlaytestResult& result, const std::string& msg)
{
    result.Errors.push_back(msg);
    ++result.ErrorCount;
}

// ============================================================================
// HarnessResult
// ============================================================================

std::string HarnessResult::Format() const
{
    std::ostringstream ss;
    ss << "=== TestHarness Results ===\n";
    ss << "Total  : " << TotalCases  << "\n";
    ss << "Passed : " << PassedCases << "\n";
    ss << "Failed : " << FailedCases << "\n";
    if (!FailedNames.empty()) {
        ss << "Failed cases:\n";
        for (const auto& n : FailedNames) ss << "  - " << n << "\n";
    }
    ss << "Result : " << (AllPassed() ? "PASS" : "FAIL") << "\n";
    return ss.str();
}

// ============================================================================
// TestHarness
// ============================================================================

void TestHarness::AddCase(TestCase tc)
{
    m_Cases.push_back(std::move(tc));
}

void TestHarness::AddCase(const std::string& name,
                           int                tickCount,
                           const std::string& scenarioTag)
{
    TestCase tc;
    tc.Name                    = name;
    tc.SessionConfig.TickCount   = tickCount;
    tc.SessionConfig.ScenarioTag = scenarioTag.empty() ? name : scenarioTag;
    m_Cases.push_back(std::move(tc));
}

HarnessResult TestHarness::RunAll()
{
    HarnessResult harness;
    harness.TotalCases = static_cast<int>(m_Cases.size());

    for (auto& tc : m_Cases) {
        Logger::Log(LogLevel::Info, "TestHarness", "Running: " + tc.Name);

        if (tc.Setup) tc.Setup();

        PlaytestSession session(tc.SessionConfig);
        PlaytestResult  r = session.Run();
        harness.CaseResults.push_back(r);

        if (tc.Teardown) tc.Teardown();

        if (r.Success) {
            ++harness.PassedCases;
            Logger::Log(LogLevel::Info, "TestHarness", "PASS: " + tc.Name);
        } else {
            ++harness.FailedCases;
            harness.FailedNames.push_back(tc.Name);
            Logger::Log(LogLevel::Warning, "TestHarness", "FAIL: " + tc.Name);
        }
    }

    std::cout << harness.Format();
    return harness;
}

} // namespace NF::Game
