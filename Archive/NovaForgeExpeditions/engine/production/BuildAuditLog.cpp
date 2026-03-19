#include "BuildAuditLog.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace atlas::production {

namespace {
    const char* SeverityLabel(AuditSeverity s) {
        switch (s) {
            case AuditSeverity::Info:    return "INFO";
            case AuditSeverity::Warning: return "WARN";
            case AuditSeverity::Error:   return "ERROR";
            case AuditSeverity::Fatal:   return "FATAL";
        }
        return "INFO";
    }

    const char* CategoryLabel(AuditCategory c) {
        switch (c) {
            case AuditCategory::Build:        return "Build";
            case AuditCategory::AssetCook:    return "AssetCook";
            case AuditCategory::Packaging:    return "Packaging";
            case AuditCategory::Verification: return "Verification";
            case AuditCategory::Upload:       return "Upload";
            case AuditCategory::Custom:       return "Custom";
        }
        return "Custom";
    }
} // anonymous namespace

void BuildAuditLog::Open(const std::string& buildId) {
    m_buildId = buildId;
    m_entries.clear();
    m_nextId = 1;
    m_durationMs = 0;
    m_open = true;
}

bool BuildAuditLog::IsOpen() const { return m_open; }

void BuildAuditLog::Log(AuditSeverity severity,
                        AuditCategory category,
                        const std::string& actor,
                        const std::string& message,
                        const std::string& detail)
{
    if (!m_open) return;

    AuditEntry entry;
    entry.id = m_nextId++;
    entry.timestampMs = static_cast<uint64_t>(m_entries.size()); // monotonic index as timestamp proxy
    entry.severity = severity;
    entry.category = category;
    entry.actor = actor;
    entry.message = message;
    entry.detail = detail;

    m_entries.push_back(entry);

    if (m_callback) m_callback(entry);
}

void BuildAuditLog::Info(const std::string& actor, const std::string& msg) {
    Log(AuditSeverity::Info, AuditCategory::Build, actor, msg);
}

void BuildAuditLog::Warning(const std::string& actor, const std::string& msg) {
    Log(AuditSeverity::Warning, AuditCategory::Build, actor, msg);
}

void BuildAuditLog::Error(const std::string& actor, const std::string& msg, const std::string& detail) {
    Log(AuditSeverity::Error, AuditCategory::Build, actor, msg, detail);
}

void BuildAuditLog::Fatal(const std::string& actor, const std::string& msg, const std::string& detail) {
    Log(AuditSeverity::Fatal, AuditCategory::Build, actor, msg, detail);
}

const std::vector<AuditEntry>& BuildAuditLog::Entries() const { return m_entries; }

size_t BuildAuditLog::EntryCount() const { return m_entries.size(); }

std::vector<AuditEntry> BuildAuditLog::FilterBySeverity(AuditSeverity sev) const {
    std::vector<AuditEntry> out;
    for (const auto& e : m_entries)
        if (e.severity == sev) out.push_back(e);
    return out;
}

std::vector<AuditEntry> BuildAuditLog::FilterByCategory(AuditCategory cat) const {
    std::vector<AuditEntry> out;
    for (const auto& e : m_entries)
        if (e.category == cat) out.push_back(e);
    return out;
}

std::vector<AuditEntry> BuildAuditLog::FilterByActor(const std::string& actor) const {
    std::vector<AuditEntry> out;
    for (const auto& e : m_entries)
        if (e.actor == actor) out.push_back(e);
    return out;
}

AuditSummary BuildAuditLog::Summary() const {
    AuditSummary s;
    s.buildId = m_buildId;
    s.durationMs = m_durationMs;
    s.totalEntries = static_cast<uint64_t>(m_entries.size());
    for (const auto& e : m_entries) {
        switch (e.severity) {
            case AuditSeverity::Info:    ++s.infoCount;    break;
            case AuditSeverity::Warning: ++s.warningCount; break;
            case AuditSeverity::Error:   ++s.errorCount;   break;
            case AuditSeverity::Fatal:   ++s.fatalCount;   break;
        }
    }
    s.hasErrors = (s.errorCount + s.fatalCount) > 0;
    return s;
}

std::string BuildAuditLog::ExportText() const {
    std::ostringstream oss;
    oss << "=== Build Audit Log: " << m_buildId << " ===\n";
    oss << "Entries: " << m_entries.size() << "\n\n";
    for (const auto& e : m_entries) {
        oss << "[" << SeverityLabel(e.severity) << "]["
            << CategoryLabel(e.category) << "]["
            << e.actor << "] " << e.message;
        if (!e.detail.empty()) oss << "\n  " << e.detail;
        oss << "\n";
    }
    return oss.str();
}

std::string BuildAuditLog::ExportJSON() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"build_id\": \"" << m_buildId << "\",\n";
    oss << "  \"entry_count\": " << m_entries.size() << ",\n";
    oss << "  \"entries\": [\n";
    for (size_t i = 0; i < m_entries.size(); ++i) {
        const auto& e = m_entries[i];
        oss << "    {\"id\":" << e.id
            << ",\"severity\":\"" << SeverityLabel(e.severity) << "\""
            << ",\"category\":\"" << CategoryLabel(e.category) << "\""
            << ",\"actor\":\"" << e.actor << "\""
            << ",\"message\":\"" << e.message << "\"}";
        if (i + 1 < m_entries.size()) oss << ",";
        oss << "\n";
    }
    oss << "  ]\n}\n";
    return oss.str();
}

bool BuildAuditLog::SaveToFile(const std::string& path) const {
    std::ofstream out(path);
    if (!out.is_open()) return false;
    out << ExportText();
    return out.good();
}

void BuildAuditLog::SetEntryCallback(EntryCallback cb) {
    m_callback = std::move(cb);
}

void BuildAuditLog::Close(uint64_t durationMs) {
    m_durationMs = durationMs;
    m_open = false;
}

void BuildAuditLog::Reset() {
    m_open = false;
    m_buildId.clear();
    m_entries.clear();
    m_nextId = 1;
    m_durationMs = 0;
    m_callback = nullptr;
}

const std::string& BuildAuditLog::BuildId() const { return m_buildId; }

} // namespace atlas::production
