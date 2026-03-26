#include "AI/BuildReport/BuildReport.h"
#include <chrono>
#include <fstream>
#include <sstream>
#include <vector>

namespace AI {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct Session {
    std::string                  tag;
    std::vector<ReportMetric>    metrics;
    std::vector<ReportSuggestion> suggestions;
    std::vector<ReportAnomaly>   anomalies;
    std::vector<std::string>     notes;
    uint64_t                     startMs{0};
    uint64_t                     endMs{0};
    bool                         active{false};
};

struct BuildReport::Impl {
    Session              current;
    std::vector<Session> completed;
};

BuildReport::BuildReport() : m_impl(new Impl()) {}
BuildReport::~BuildReport() { delete m_impl; }

void BuildReport::Init()     {}
void BuildReport::Shutdown() {}

void BuildReport::BeginSession(const std::string& tag) {
    m_impl->current         = Session{};
    m_impl->current.tag     = tag;
    m_impl->current.startMs = NowMs();
    m_impl->current.active  = true;
}

void BuildReport::EndSession() {
    if (!m_impl->current.active) return;
    m_impl->current.endMs  = NowMs();
    m_impl->current.active = false;
    m_impl->completed.push_back(m_impl->current);
}

bool BuildReport::IsSessionActive() const { return m_impl->current.active; }

void BuildReport::RecordMetric(const std::string& name, float value,
                               const std::string& unit) {
    m_impl->current.metrics.push_back({name, value, unit});
}

void BuildReport::RecordSuggestion(const std::string& description,
                                   bool accepted, float confidence) {
    m_impl->current.suggestions.push_back({description, accepted, confidence});
}

void BuildReport::RecordAnomaly(const std::string& metric, float value,
                                const std::string& reason) {
    m_impl->current.anomalies.push_back({metric, value, reason});
}

void BuildReport::RecordNote(const std::string& text) {
    m_impl->current.notes.push_back(text);
}

std::string BuildReport::ToMarkdown() const {
    const Session& s = m_impl->current.active
                       ? m_impl->current
                       : (m_impl->completed.empty() ? m_impl->current
                                                    : m_impl->completed.back());
    std::ostringstream ss;
    ss << "# Build Report — " << s.tag << "\n\n";

    if (!s.metrics.empty()) {
        ss << "## Metrics\n\n";
        ss << "| Metric | Value | Unit |\n|--------|-------|------|\n";
        for (auto& m : s.metrics)
            ss << "| " << m.name << " | " << m.value << " | " << m.unit << " |\n";
        ss << "\n";
    }

    if (!s.suggestions.empty()) {
        ss << "## AI Suggestions\n\n";
        ss << "| Description | Accepted | Confidence |\n"
              "|-------------|----------|------------|\n";
        for (auto& sg : s.suggestions)
            ss << "| " << sg.description
               << " | " << (sg.accepted ? "✅" : "❌")
               << " | " << sg.confidence << " |\n";
        ss << "\n";
    }

    if (!s.anomalies.empty()) {
        ss << "## Anomalies\n\n";
        for (auto& a : s.anomalies)
            ss << "- **" << a.metric << "** = " << a.value
               << " — " << a.reason << "\n";
        ss << "\n";
    }

    if (!s.notes.empty()) {
        ss << "## Notes\n\n";
        for (auto& n : s.notes) ss << "- " << n << "\n";
        ss << "\n";
    }

    uint64_t dur = s.endMs > s.startMs ? s.endMs - s.startMs : 0;
    ss << "_Session duration: " << dur / 1000 << "s_\n";
    return ss.str();
}

bool BuildReport::SaveMarkdown(const std::string& filePath) const {
    std::ofstream f(filePath);
    if (!f.is_open()) return false;
    f << ToMarkdown();
    return true;
}

std::string BuildReport::Summary() const {
    const Session& s = m_impl->current.active ? m_impl->current
                       : (m_impl->completed.empty() ? m_impl->current
                                                    : m_impl->completed.back());
    uint32_t accepted = 0;
    for (auto& sg : s.suggestions) if (sg.accepted) accepted++;
    std::ostringstream ss;
    ss << "[BuildReport] " << s.tag
       << " | metrics=" << s.metrics.size()
       << " suggestions=" << s.suggestions.size()
       << " (accepted=" << accepted << ")"
       << " anomalies=" << s.anomalies.size();
    return ss.str();
}

std::vector<std::string> BuildReport::PreviousSessions() const {
    std::vector<std::string> out;
    out.reserve(m_impl->completed.size());
    for (auto& s : m_impl->completed) out.push_back(s.tag);
    return out;
}

void BuildReport::ClearHistory() {
    m_impl->completed.clear();
}

} // namespace AI
