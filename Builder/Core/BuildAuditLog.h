#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace Builder {

enum class AuditSeverity : uint8_t {
    Info    = 0,
    Warning = 1,
    Error   = 2,
    Fatal   = 3,
};

enum class AuditCategory : uint8_t {
    Build        = 0,
    AssetCook    = 1,
    Packaging    = 2,
    Verification = 3,
    Upload       = 4,
    Custom       = 5,
};

struct AuditEntry {
    uint64_t      id          = 0;
    uint64_t      timestampMs = 0;
    AuditSeverity severity    = AuditSeverity::Info;
    AuditCategory category    = AuditCategory::Build;
    std::string   actor;
    std::string   message;
    std::string   detail;
};

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

class BuildAuditLog {
public:
    using EntryCallback = std::function<void(const AuditEntry&)>;

    void Open(const std::string& buildId);
    bool IsOpen() const;

    void Log(AuditSeverity severity, AuditCategory category,
             const std::string& actor, const std::string& message,
             const std::string& detail = "");

    void Info   (const std::string& actor, const std::string& msg);
    void Warning(const std::string& actor, const std::string& msg);
    void Error  (const std::string& actor, const std::string& msg, const std::string& detail = "");
    void Fatal  (const std::string& actor, const std::string& msg, const std::string& detail = "");

    const std::vector<AuditEntry>& Entries() const;
    size_t EntryCount() const;

    std::vector<AuditEntry> FilterBySeverity(AuditSeverity sev) const;
    std::vector<AuditEntry> FilterByCategory(AuditCategory cat) const;
    std::vector<AuditEntry> FilterByActor(const std::string& actor) const;

    AuditSummary Summary() const;

    std::string ExportText() const;
    std::string ExportJSON() const;
    bool        SaveToFile(const std::string& path) const;

    void SetEntryCallback(EntryCallback cb);
    void Close(uint64_t durationMs = 0);
    void Reset();

    const std::string& BuildId() const;

private:
    std::string             m_buildId;
    bool                    m_open       = false;
    uint64_t                m_durationMs = 0;
    std::vector<AuditEntry> m_entries;
    uint64_t                m_nextId     = 1;
    EntryCallback           m_callback;
};

} // namespace Builder
