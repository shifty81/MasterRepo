#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace AI {

// ──────────────────────────────────────────────────────────────
// Bug severity / status
// ──────────────────────────────────────────────────────────────

enum class BugSeverity  { Critical, High, Medium, Low, Info };
enum class BugStatus    { Open, InProgress, Resolved, Duplicate, WontFix };

// ──────────────────────────────────────────────────────────────
// Bug report (raw input from crash log, CI failure, user report)
// ──────────────────────────────────────────────────────────────

struct BugReport {
    std::string  id;
    std::string  title;
    std::string  description;
    std::string  stackTrace;
    std::string  logSnippet;
    std::string  filePath;
    uint32_t     lineNumber = 0;
    std::string  submittedAt;
};

// ──────────────────────────────────────────────────────────────
// Triage result produced by AI
// ──────────────────────────────────────────────────────────────

struct TriageResult {
    std::string              reportId;
    BugSeverity              severity       = BugSeverity::Medium;
    BugStatus                status         = BugStatus::Open;
    std::string              summary;
    std::string              suggestedFix;
    std::vector<std::string> relatedFiles;
    std::vector<std::string> similarBugIds; // de-dup candidates
    float                    confidence     = 0.f;  // 0.0 – 1.0
};

// ──────────────────────────────────────────────────────────────
// BugTriage — AI-assisted bug classification and fix suggestions
// ──────────────────────────────────────────────────────────────

class BugTriage {
public:
    // Submit a bug report for AI triage
    TriageResult Triage(const BugReport& report);

    // Batch triage from a crash log file
    std::vector<TriageResult> TriageFromLog(const std::string& logFilePath);

    // Bulk triage from a directory of log files
    std::vector<TriageResult> TriageDirectory(const std::string& logDir);

    // Duplicate detection across all known bugs
    std::vector<std::string> FindDuplicates(const BugReport& report) const;

    // Persist / load known bugs
    void   LoadBugDatabase(const std::string& filePath);
    void   SaveBugDatabase(const std::string& filePath) const;

    // Update status of a bug
    void   UpdateStatus(const std::string& bugId, BugStatus status);

    // Query
    std::vector<TriageResult> GetByStatus(BugStatus status) const;
    std::vector<TriageResult> GetBySeverity(BugSeverity severity) const;
    size_t OpenBugCount()     const;
    size_t ResolvedBugCount() const;

    // Callback when a new triage result is ready
    using TriageCallback = std::function<void(const TriageResult&)>;
    void OnTriageComplete(TriageCallback cb);

    // Singleton
    static BugTriage& Get();

private:
    BugSeverity  ClassifySeverity(const BugReport& report) const;
    std::string  SuggestFix(const BugReport& report) const;
    std::string  GenerateSummary(const BugReport& report) const;

    std::vector<TriageResult>   m_triaged;
    TriageCallback              m_onComplete;
};

} // namespace AI
