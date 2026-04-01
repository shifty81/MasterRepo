#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace NF::Game {

/// @brief Severity level for a single subsystem health record.
enum class HealthSeverity : uint8_t {
    Pass  = 0, ///< Subsystem initialised and ticking normally.
    Warn  = 1, ///< Subsystem functional but with caveats.
    Error = 2, ///< Subsystem failed or produced errors.
};

/// @brief Health record for one engine subsystem.
struct SubsystemHealth {
    std::string    Name;
    HealthSeverity Severity = HealthSeverity::Pass;
    std::string    Message;

    /// @brief Returns "pass", "warn", or "error".
    [[nodiscard]] std::string SeverityString() const noexcept;
};

/// @brief Collects per-subsystem health data and serialises it to JSON.
///
/// Typical usage:
/// @code
///   Diagnostics diag("session-001");
///   diag.RecordSubsystem("Renderer", HealthSeverity::Pass, "OK");
///   diag.WriteReport("diagnostics.json");
/// @endcode
class Diagnostics {
public:
    /// @brief Construct with a session identifier embedded in the report.
    explicit Diagnostics(std::string sessionId);

    // -------------------------------------------------------------------------
    // Recording
    // -------------------------------------------------------------------------

    /// @brief Record the health of a single subsystem.
    void RecordSubsystem(std::string name,
                         HealthSeverity severity,
                         std::string message);

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    /// @brief Returns true if any subsystem reported an Error severity.
    [[nodiscard]] bool HasErrors() const noexcept;

    // -------------------------------------------------------------------------
    // Serialisation
    // -------------------------------------------------------------------------

    /// @brief Serialise the report to an in-memory JSON string.
    [[nodiscard]] std::string ToJson() const;

    /// @brief Write the JSON report to @p outputPath.
    /// @return True on success; false when the file cannot be opened.
    bool WriteReport(const std::string& outputPath) const;

    // -------------------------------------------------------------------------
    // Factory
    // -------------------------------------------------------------------------

    /// @brief Build a @c Diagnostics report from a completed playtest result.
    /// @param sessionId   Identifier embedded in the report.
    /// @param passed      True when the playtest session passed.
    /// @param errorCount  Number of errors recorded.
    /// @param errors      Individual error strings.
    static Diagnostics FromPlaytestResult(
        const std::string&              sessionId,
        bool                            passed,
        int                             errorCount,
        const std::vector<std::string>& errors);

private:
    std::string                 m_SessionId;
    std::vector<SubsystemHealth> m_Subsystems;
};

} // namespace NF::Game
