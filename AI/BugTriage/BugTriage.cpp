#include "AI/BugTriage/BugTriage.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

namespace AI {

BugTriage& BugTriage::Get() {
    static BugTriage instance;
    return instance;
}

// ── Triage ─────────────────────────────────────────────────────

TriageResult BugTriage::Triage(const BugReport& report) {
    TriageResult result;
    result.reportId    = report.id;
    result.severity    = ClassifySeverity(report);
    result.status      = BugStatus::Open;
    result.summary     = GenerateSummary(report);
    result.suggestedFix = SuggestFix(report);
    result.confidence  = 0.75f;

    // File references from stack trace
    static const std::regex fileRef(R"(([a-zA-Z0-9_/\\\.]+\.(?:cpp|h|hpp)):(\d+))");
    std::sregex_iterator it(report.stackTrace.begin(), report.stackTrace.end(), fileRef);
    std::sregex_iterator end;
    for (; it != end; ++it) {
        result.relatedFiles.push_back((*it)[1].str());
    }

    // Duplicate detection
    result.similarBugIds = FindDuplicates(report);
    m_triaged.push_back(result);
    if (m_onComplete) m_onComplete(result);
    return result;
}

std::vector<TriageResult> BugTriage::TriageFromLog(const std::string& logFilePath) {
    std::vector<TriageResult> results;
    std::ifstream f(logFilePath);
    if (!f.is_open()) return results;

    // Simple heuristic: split on "CRASH" / "ERROR" / "ASSERT" markers
    static const std::regex marker(R"((CRASH|ASSERT|FATAL|ERROR):([^\n]+))");
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    std::sregex_iterator it(content.begin(), content.end(), marker);
    std::sregex_iterator end;
    int idx = 0;
    for (; it != end; ++it, ++idx) {
        BugReport report;
        report.id          = logFilePath + "_" + std::to_string(idx);
        report.title       = (*it)[1].str() + ": " + (*it)[2].str();
        report.description = (*it)[0].str();
        report.logSnippet  = content.substr(
            static_cast<size_t>(it->position()),
            std::min<size_t>(512, content.size() - static_cast<size_t>(it->position())));
        results.push_back(Triage(report));
    }
    return results;
}

std::vector<TriageResult> BugTriage::TriageDirectory(const std::string& logDir) {
    std::vector<TriageResult> all;
    if (!fs::exists(logDir)) return all;
    for (auto& entry : fs::recursive_directory_iterator(logDir)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        if (ext == ".log" || ext == ".txt") {
            auto results = TriageFromLog(entry.path().string());
            all.insert(all.end(), results.begin(), results.end());
        }
    }
    return all;
}

// ── Duplicate detection ────────────────────────────────────────

std::vector<std::string> BugTriage::FindDuplicates(const BugReport& report) const {
    constexpr size_t kSimilarityPrefixLength = 20;
    std::vector<std::string> dupes;
    for (const auto& t : m_triaged) {
        if (t.reportId == report.id) continue;
        // Simple similarity: shared title prefix in summary
        if (!t.summary.empty() && !report.title.empty() &&
            t.summary.find(report.title.substr(0, std::min(kSimilarityPrefixLength, report.title.size())))
                != std::string::npos) {
            dupes.push_back(t.reportId);
        }
    }
    return dupes;
}

// ── Persist ────────────────────────────────────────────────────

void BugTriage::LoadBugDatabase(const std::string& /*filePath*/) {
    // Delegated to Core::Serializer in production
}

void BugTriage::SaveBugDatabase(const std::string& /*filePath*/) const {
    // Delegated to Core::Serializer in production
}

void BugTriage::UpdateStatus(const std::string& bugId, BugStatus status) {
    for (auto& t : m_triaged) {
        if (t.reportId == bugId) {
            t.status = status;
            return;
        }
    }
}

// ── Query ──────────────────────────────────────────────────────

std::vector<TriageResult> BugTriage::GetByStatus(BugStatus status) const {
    std::vector<TriageResult> out;
    for (const auto& t : m_triaged) {
        if (t.status == status) out.push_back(t);
    }
    return out;
}

std::vector<TriageResult> BugTriage::GetBySeverity(BugSeverity severity) const {
    std::vector<TriageResult> out;
    for (const auto& t : m_triaged) {
        if (t.severity == severity) out.push_back(t);
    }
    return out;
}

size_t BugTriage::OpenBugCount() const {
    return static_cast<size_t>(
        std::count_if(m_triaged.begin(), m_triaged.end(),
                      [](const TriageResult& t){ return t.status == BugStatus::Open; }));
}

size_t BugTriage::ResolvedBugCount() const {
    return static_cast<size_t>(
        std::count_if(m_triaged.begin(), m_triaged.end(),
                      [](const TriageResult& t){ return t.status == BugStatus::Resolved; }));
}

void BugTriage::OnTriageComplete(TriageCallback cb) {
    m_onComplete = std::move(cb);
}

// ── Private helpers ────────────────────────────────────────────

BugSeverity BugTriage::ClassifySeverity(const BugReport& report) const {
    const std::string& text = report.stackTrace + report.description + report.title;
    if (text.find("CRASH") != std::string::npos ||
        text.find("segfault") != std::string::npos ||
        text.find("abort") != std::string::npos)    return BugSeverity::Critical;
    if (text.find("ASSERT") != std::string::npos ||
        text.find("nullptr") != std::string::npos)  return BugSeverity::High;
    if (text.find("WARNING") != std::string::npos)  return BugSeverity::Medium;
    if (text.find("TODO") != std::string::npos ||
        text.find("FIXME") != std::string::npos)    return BugSeverity::Low;
    return BugSeverity::Info;
}

std::string BugTriage::SuggestFix(const BugReport& report) const {
    const std::string& text = report.stackTrace + report.description;
    if (text.find("nullptr") != std::string::npos)
        return "Add null check before dereferencing pointer.";
    if (text.find("out_of_range") != std::string::npos ||
        text.find("index") != std::string::npos)
        return "Validate array/container bounds before access.";
    if (text.find("stack overflow") != std::string::npos)
        return "Check for infinite recursion or excessively deep call stack.";
    if (text.find("memory") != std::string::npos)
        return "Run address sanitizer to detect memory corruption or leaks.";
    return "Review the stack trace and add defensive checks at the indicated location.";
}

std::string BugTriage::GenerateSummary(const BugReport& report) const {
    if (!report.title.empty()) return report.title;
    if (report.description.size() > 80)
        return report.description.substr(0, 80) + "...";
    return report.description;
}

} // namespace AI
