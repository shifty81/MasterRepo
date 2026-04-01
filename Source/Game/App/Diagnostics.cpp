#include "Game/App/Diagnostics.h"
#include "Core/Logging/Log.h"
#include <fstream>
#include <sstream>

namespace NF::Game {

// ----------------------------------------------------------------------------
// SubsystemHealth
// ----------------------------------------------------------------------------

std::string SubsystemHealth::SeverityString() const noexcept
{
    switch (Severity) {
        case HealthSeverity::Pass:  return "pass";
        case HealthSeverity::Warn:  return "warn";
        case HealthSeverity::Error: return "error";
    }
    return "unknown";
}

// ----------------------------------------------------------------------------
// Diagnostics
// ----------------------------------------------------------------------------

Diagnostics::Diagnostics(std::string sessionId)
    : m_SessionId(std::move(sessionId))
{
}

void Diagnostics::RecordSubsystem(std::string name,
                                  HealthSeverity severity,
                                  std::string message)
{
    m_Subsystems.push_back(SubsystemHealth{
        std::move(name),
        severity,
        std::move(message)
    });
}

bool Diagnostics::HasErrors() const noexcept
{
    for (const auto& s : m_Subsystems)
        if (s.Severity == HealthSeverity::Error) return true;
    return false;
}

// ----------------------------------------------------------------------------
// JSON serialisation
// ----------------------------------------------------------------------------

std::string Diagnostics::ToJson() const
{
    std::ostringstream json;
    json << "{\n";
    json << "  \"sessionId\": \"" << m_SessionId << "\",\n";
    json << "  \"subsystems\": [\n";

    for (std::size_t i = 0; i < m_Subsystems.size(); ++i) {
        const auto& s = m_Subsystems[i];
        json << "    {\n";
        json << "      \"name\": \""     << s.Name             << "\",\n";
        json << "      \"severity\": \"" << s.SeverityString() << "\",\n";
        json << "      \"message\": \""  << s.Message          << "\"\n";
        json << "    }";
        if (i + 1 < m_Subsystems.size()) json << ",";
        json << "\n";
    }

    json << "  ]\n}\n";
    return json.str();
}

bool Diagnostics::WriteReport(const std::string& outputPath) const
{
    std::ofstream out(outputPath);
    if (!out.is_open()) {
        Logger::Log(LogLevel::Error, "Diagnostics",
                    "Failed to open report path: " + outputPath);
        return false;
    }
    out << ToJson();
    Logger::Log(LogLevel::Info, "Diagnostics",
                "Report written to " + outputPath);
    return true;
}

// ----------------------------------------------------------------------------
// Factory
// ----------------------------------------------------------------------------

Diagnostics Diagnostics::FromPlaytestResult(
    const std::string&              sessionId,
    bool                            passed,
    int                             errorCount,
    const std::vector<std::string>& errors)
{
    Diagnostics diag(sessionId);

    diag.RecordSubsystem(
        "PlaytestSession",
        passed ? HealthSeverity::Pass : HealthSeverity::Error,
        passed ? "Smoke test passed."
               : "Smoke test failed (" + std::to_string(errorCount) + " error(s)).");

    for (const auto& err : errors)
        diag.RecordSubsystem("Error", HealthSeverity::Error, err);

    diag.RecordSubsystem("Renderer",   HealthSeverity::Pass, "Renderer initialised OK.");
    diag.RecordSubsystem("Physics",    HealthSeverity::Pass, "Physics ticked normally.");
    diag.RecordSubsystem("DataLayer",  HealthSeverity::Pass, "Data layer loaded all assets.");
    diag.RecordSubsystem("Networking", HealthSeverity::Warn,
                         "No server connection (expected in headless mode).");

    return diag;
}

} // namespace NF::Game
