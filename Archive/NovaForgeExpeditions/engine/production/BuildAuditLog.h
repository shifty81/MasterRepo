#pragma once
// ============================================================
// Atlas Build Audit Log — Per-build audit trail for CI/CD
// ============================================================
//
// Records every significant event that occurs during a build
// (configuration, compilation, asset cooking, packaging,
// verification) so that any failure or anomaly can be traced
// back to a specific step, timestamp, and responsible actor.
//
// The audit log is separate from the CertifiedBuildSystem:
// the certified build checks *pass/fail*, while the audit log
// records *what happened and when* at fine granularity.
//
// See: docs/12_CI_AND_BUILD.md
//      docs/ATLAS_CORE_CONTRACT.md

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace atlas::production {

/// Severity of an audit log entry.
enum class AuditSeverity : uint8_t {
    Info    = 0,
    Warning = 1,
    Error   = 2,
    Fatal   = 3,
};

/// Category of work being performed.
enum class AuditCategory : uint8_t {
    Build       = 0,
    AssetCook   = 1,
    Packaging   = 2,
    Verification = 3,
    Upload      = 4,
    Custom      = 5,
};

/// A single audit log entry.
struct AuditEntry {
    uint64_t      id = 0;
    uint64_t      timestampMs = 0;  ///< Milliseconds since audit log was opened
    AuditSeverity severity = AuditSeverity::Info;
    AuditCategory category = AuditCategory::Build;
    std::string   actor;            ///< Who performed the action (step name / CI job)
    std::string   message;
    std::string   detail;           ///< Optional extended detail / stack trace
};

/// Summary statistics for a completed audit session.
struct AuditSummary {
    uint64_t totalEntries = 0;
    uint64_t infoCount    = 0;
    uint64_t warningCount = 0;
    uint64_t errorCount   = 0;
    uint64_t fatalCount   = 0;
    bool     hasErrors    = false;
    std::string buildId;
    uint64_t durationMs   = 0;
};

/// Immutable build audit log.  Records events in append-only order.
class BuildAuditLog {
public:
    using EntryCallback = std::function<void(const AuditEntry&)>;

    /// Open the log for a new build session.  Clears any previous entries.
    void Open(const std::string& buildId);

    /// Close the log.  After closing, no new entries may be added.
    bool IsOpen() const;

    /// Append an entry.  No-op if the log is closed.
    void Log(AuditSeverity severity,
             AuditCategory category,
             const std::string& actor,
             const std::string& message,
             const std::string& detail = "");

    /// Convenience helpers.
    void Info   (const std::string& actor, const std::string& msg);
    void Warning(const std::string& actor, const std::string& msg);
    void Error  (const std::string& actor, const std::string& msg, const std::string& detail = "");
    void Fatal  (const std::string& actor, const std::string& msg, const std::string& detail = "");

    /// All recorded entries, in order.
    const std::vector<AuditEntry>& Entries() const;

    /// Number of entries.
    size_t EntryCount() const;

    /// Filter entries by severity or category.
    std::vector<AuditEntry> FilterBySeverity(AuditSeverity sev) const;
    std::vector<AuditEntry> FilterByCategory(AuditCategory cat) const;
    std::vector<AuditEntry> FilterByActor(const std::string& actor) const;

    /// Build-wide summary statistics.
    AuditSummary Summary() const;

    /// Export the audit log as a plain-text report.
    std::string ExportText() const;

    /// Export the audit log as JSON.
    std::string ExportJSON() const;

    /// Save to a file.  Returns true on success.
    bool SaveToFile(const std::string& path) const;

    /// Register an optional real-time callback invoked on every new entry.
    void SetEntryCallback(EntryCallback cb);

    /// Close the log and record the build duration.
    void Close(uint64_t durationMs = 0);

    /// Clear all entries and reset to unopened state.
    void Reset();

    const std::string& BuildId() const;

private:
    std::string          m_buildId;
    bool                 m_open = false;
    uint64_t             m_durationMs = 0;
    std::vector<AuditEntry> m_entries;
    uint64_t             m_nextId = 1;
    EntryCallback        m_callback;
};

} // namespace atlas::production
