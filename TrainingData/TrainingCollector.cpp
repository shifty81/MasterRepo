#include "TrainingData/TrainingCollector.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace AI {

// ──────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────

static uint64_t TCNowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

static std::string TCEscapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:   out += c;      break;
        }
    }
    return out;
}

static const char* KindToStr(SampleKind k) {
    switch (k) {
    case SampleKind::CodeSnippet:    return "code";
    case SampleKind::ErrorFix:       return "error_fix";
    case SampleKind::AIPrompt:       return "ai_prompt";
    case SampleKind::AssetDescription: return "asset_description";
    case SampleKind::Diff:           return "diff";
    }
    return "unknown";
}

// ──────────────────────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────────────────────

TrainingCollector::TrainingCollector(const std::string& dataRoot)
    : m_root(dataRoot) {
    if (!m_root.empty() && m_root.back() != '/') m_root += '/';
}

// ──────────────────────────────────────────────────────────────
// Collect
// ──────────────────────────────────────────────────────────────

void TrainingCollector::AddSample(RawSample s) {
    if (s.timestamp == 0) s.timestamp = TCNowMs();
    m_samples.push_back(std::move(s));
}

void TrainingCollector::CollectFromDirectory(const std::string& dir, SampleKind kind) {
    namespace fs = std::filesystem;
    if (!fs::exists(dir)) return;

    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;

        std::ifstream f(entry.path());
        if (!f) continue;
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        if (content.empty()) continue;

        RawSample s;
        s.kind       = kind;
        s.input      = content;
        s.output     = "";
        s.sourceFile = entry.path().string();
        s.tag        = entry.path().filename().string();
        s.timestamp  = TCNowMs();
        m_samples.push_back(std::move(s));
    }
}

// ──────────────────────────────────────────────────────────────
// Filter & clean
// ──────────────────────────────────────────────────────────────

void TrainingCollector::FilterMinLength(size_t minInputLen, size_t minOutputLen) {
    m_samples.erase(
        std::remove_if(m_samples.begin(), m_samples.end(),
            [&](const RawSample& s) {
                return s.input.size()  < minInputLen
                    || s.output.size() < minOutputLen;
            }),
        m_samples.end());
}

void TrainingCollector::DeduplicateByInput() {
    std::vector<RawSample> unique;
    unique.reserve(m_samples.size());
    for (auto& s : m_samples) {
        bool found = false;
        for (const auto& u : unique) {
            if (u.input == s.input) { found = true; break; }
        }
        if (!found) unique.push_back(std::move(s));
    }
    m_samples = std::move(unique);
}

// ──────────────────────────────────────────────────────────────
// Export
// ──────────────────────────────────────────────────────────────

bool TrainingCollector::writeSamples(const std::vector<RawSample>& samples,
                                      const std::string& path) const {
    std::ofstream out(path);
    if (!out) return false;
    for (const auto& s : samples) {
        out << "{\"input\":\""  << TCEscapeJson(s.input)  << "\","
            << "\"output\":\""  << TCEscapeJson(s.output) << "\","
            << "\"kind\":\""    << KindToStr(s.kind)      << "\","
            << "\"tag\":\""     << TCEscapeJson(s.tag)    << "\","
            << "\"source\":\""  << TCEscapeJson(s.sourceFile) << "\","
            << "\"timestamp\":" << s.timestamp
            << "}\n";
    }
    return true;
}

bool TrainingCollector::ExportCodeSamples(const std::string& outPath) const {
    std::vector<RawSample> filtered;
    for (const auto& s : m_samples)
        if (s.kind == SampleKind::CodeSnippet) filtered.push_back(s);
    return writeSamples(filtered, outPath);
}

bool TrainingCollector::ExportErrorFixes(const std::string& outPath) const {
    std::vector<RawSample> filtered;
    for (const auto& s : m_samples)
        if (s.kind == SampleKind::ErrorFix) filtered.push_back(s);
    return writeSamples(filtered, outPath);
}

bool TrainingCollector::ExportAll(const std::string& outPath) const {
    return writeSamples(m_samples, outPath);
}

// ──────────────────────────────────────────────────────────────
// Stats
// ──────────────────────────────────────────────────────────────

CollectorStats TrainingCollector::GetStats() const {
    CollectorStats stats;
    stats.total = m_samples.size();
    for (const auto& s : m_samples) {
        switch (s.kind) {
        case SampleKind::CodeSnippet:      ++stats.code;       break;
        case SampleKind::ErrorFix:         ++stats.errorFixes; break;
        case SampleKind::AIPrompt:         ++stats.prompts;    break;
        case SampleKind::AssetDescription: ++stats.assets;     break;
        case SampleKind::Diff:             ++stats.diffs;      break;
        }
    }
    return stats;
}

size_t TrainingCollector::Count() const { return m_samples.size(); }
void   TrainingCollector::Clear()       { m_samples.clear(); }

} // namespace AI
