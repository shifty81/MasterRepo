#include "IDE/OutputPanel/OutputPanel/OutputPanel.h"
#include <algorithm>
#include <chrono>
#include <deque>
#include <sstream>

namespace IDE {

const char* SeverityName(OutputSeverity s) {
    switch (s) {
    case OutputSeverity::Debug:   return "Debug";
    case OutputSeverity::Info:    return "Info";
    case OutputSeverity::Warning: return "Warning";
    case OutputSeverity::Error:   return "Error";
    default:                      return "Info";
    }
}

static uint64_t nowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

struct OutputPanel::Impl {
    std::deque<OutputLine>     lines;
    uint32_t                   maxLines{10000};
    uint64_t                   nextId{1};
    uint32_t                   lineCounter{0};
    uint32_t                   errorCount{0};
    uint32_t                   warningCount{0};
    bool                       tailMode{false};
    uint32_t                   tailLines{200};
    std::vector<NewLineCb>     cbs;
};

OutputPanel::OutputPanel()           : m_impl(new Impl()) {}
OutputPanel::OutputPanel(uint32_t max): m_impl(new Impl()) { m_impl->maxLines = max; }
OutputPanel::~OutputPanel()           { delete m_impl; }

void OutputPanel::Append(const std::string& text, OutputSeverity sev, const std::string& cat) {
    OutputLine line;
    line.id          = m_impl->nextId++;
    line.text        = text;
    line.severity    = sev;
    line.category    = cat;
    line.timestampMs = nowMs();
    line.lineNumber  = ++m_impl->lineCounter;
    if (sev==OutputSeverity::Error)   ++m_impl->errorCount;
    if (sev==OutputSeverity::Warning) ++m_impl->warningCount;

    m_impl->lines.push_back(line);
    while (m_impl->lines.size() > m_impl->maxLines) {
        const auto& old = m_impl->lines.front();
        if (old.severity==OutputSeverity::Error)   --m_impl->errorCount;
        if (old.severity==OutputSeverity::Warning) --m_impl->warningCount;
        m_impl->lines.pop_front();
    }
    for (auto& cb : m_impl->cbs) cb(line);
}

void OutputPanel::AppendBatch(const std::vector<std::string>& lines,
                               OutputSeverity sev, const std::string& cat) {
    for (const auto& l : lines) Append(l, sev, cat);
}

std::vector<OutputLine> OutputPanel::GetLines(std::optional<uint32_t> tail) const {
    if (tail) {
        uint32_t n = std::min(*tail, static_cast<uint32_t>(m_impl->lines.size()));
        return {m_impl->lines.end()-n, m_impl->lines.end()};
    }
    if (m_impl->tailMode) {
        uint32_t n = std::min(m_impl->tailLines, static_cast<uint32_t>(m_impl->lines.size()));
        return {m_impl->lines.end()-n, m_impl->lines.end()};
    }
    return {m_impl->lines.begin(), m_impl->lines.end()};
}

std::vector<OutputLine> OutputPanel::Filter(OutputSeverity sev, const std::string& cat) const {
    std::vector<OutputLine> out;
    for (const auto& l : m_impl->lines) {
        if (l.severity != sev) continue;
        if (!cat.empty() && l.category != cat) continue;
        out.push_back(l);
    }
    return out;
}

std::vector<OutputLine> OutputPanel::Search(const std::string& q, bool cs) const {
    std::vector<OutputLine> out;
    auto toLower=[](std::string s){ for(auto&c:s) c=static_cast<char>(::tolower(c)); return s; };
    std::string lq = cs ? q : toLower(q);
    for (const auto& l : m_impl->lines) {
        std::string t = cs ? l.text : toLower(l.text);
        if (t.find(lq) != std::string::npos) out.push_back(l);
    }
    return out;
}

const OutputLine* OutputPanel::LastLine() const {
    return m_impl->lines.empty() ? nullptr : &m_impl->lines.back();
}

size_t OutputPanel::TotalCount()   const { return m_impl->lineCounter; }
size_t OutputPanel::CurrentCount() const { return m_impl->lines.size(); }
uint32_t OutputPanel::ErrorCount()   const { return m_impl->errorCount; }
uint32_t OutputPanel::WarningCount() const { return m_impl->warningCount; }

void OutputPanel::Clear() {
    m_impl->lines.clear();
    m_impl->errorCount   = 0;
    m_impl->warningCount = 0;
}

void OutputPanel::SetTailMode(bool en, uint32_t tail) {
    m_impl->tailMode  = en;
    m_impl->tailLines = tail;
}
bool OutputPanel::TailMode() const { return m_impl->tailMode; }

void OutputPanel::SetMaxLines(uint32_t max) { m_impl->maxLines = max; }
uint32_t OutputPanel::MaxLines() const { return m_impl->maxLines; }

std::string OutputPanel::ExportText(std::optional<OutputSeverity> sevFilter) const {
    std::ostringstream ss;
    for (const auto& l : m_impl->lines) {
        if (sevFilter && l.severity != *sevFilter) continue;
        ss << "[" << SeverityName(l.severity) << "]";
        if (!l.category.empty()) ss << "[" << l.category << "]";
        ss << " " << l.text << "\n";
    }
    return ss.str();
}

std::string OutputPanel::ExportCSV() const {
    std::ostringstream ss;
    ss << "id,severity,category,timestampMs,lineNumber,text\n";
    for (const auto& l : m_impl->lines) {
        ss << l.id << "," << SeverityName(l.severity) << ","
           << l.category << "," << l.timestampMs << ","
           << l.lineNumber << ",\"" << l.text << "\"\n";
    }
    return ss.str();
}

void OutputPanel::OnNewLine(NewLineCb cb) { m_impl->cbs.push_back(std::move(cb)); }

} // namespace IDE
