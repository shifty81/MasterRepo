#include "IDE/Console/ConsoleFilter/ConsoleFilter.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace IDE {

// ── helpers ───────────────────────────────────────────────────────────────────

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

static std::string ToLowerStr(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return r;
}

static const char* LevelName(LogLevel l) {
    switch (l) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARNING";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

// ── Impl ─────────────────────────────────────────────────────────────────────

struct ConsoleFilter::Impl {
    std::vector<ConsoleEntry>   all;
    FilterConfig                config;
    std::function<void()>       onFiltered;

    // Rebuild cached regex on config changes
    mutable bool                regexDirty{true};
    mutable std::regex          compiledRegex;
    mutable bool                regexValid{false};

    void CompileRegex() const {
        regexDirty = false;
        regexValid = false;
        if (!config.regexEnabled || config.textFilter.empty()) return;
        try {
            auto flags = std::regex::ECMAScript;
            if (!config.caseSensitive) flags |= std::regex::icase;
            compiledRegex = std::regex(config.textFilter, flags);
            regexValid = true;
        } catch (...) {}
    }
};

// ── ConsoleFilter ─────────────────────────────────────────────────────────────

ConsoleFilter::ConsoleFilter() : m_impl(new Impl{}) {}
ConsoleFilter::~ConsoleFilter() { delete m_impl; }

void ConsoleFilter::AddEntry(const ConsoleEntry& entry) {
    m_impl->all.push_back(entry);
    if (m_impl->all.back().timestamp == 0)
        m_impl->all.back().timestamp = NowMs();
    // No notify — caller should batch; notify via SetConfig or explicit calls
}

void ConsoleFilter::Clear() {
    m_impl->all.clear();
    NotifyFiltered();
}

void ConsoleFilter::SetConfig(const FilterConfig& config) {
    m_impl->config     = config;
    m_impl->regexDirty = true;
    NotifyFiltered();
}

const FilterConfig& ConsoleFilter::GetConfig() const { return m_impl->config; }

std::vector<ConsoleEntry> ConsoleFilter::FilteredEntries() const {
    std::vector<ConsoleEntry> result;
    result.reserve(m_impl->all.size());
    for (const auto& e : m_impl->all)
        if (MatchesFilter(e)) result.push_back(e);
    return result;
}

const std::vector<ConsoleEntry>& ConsoleFilter::AllEntries() const { return m_impl->all; }

size_t ConsoleFilter::EntryCount()    const { return m_impl->all.size(); }
size_t ConsoleFilter::FilteredCount() const {
    size_t n = 0;
    for (const auto& e : m_impl->all) if (MatchesFilter(e)) ++n;
    return n;
}

void ConsoleFilter::SetTextFilter(const std::string& text) {
    m_impl->config.textFilter = text;
    m_impl->regexDirty = true;
    NotifyFiltered();
}

void ConsoleFilter::SetLevelFilter(LogLevel level, bool enabled) {
    if (enabled) m_impl->config.enabledLevels.insert(level);
    else         m_impl->config.enabledLevels.erase(level);
    NotifyFiltered();
}

void ConsoleFilter::EnableRegex(bool enable) {
    m_impl->config.regexEnabled = enable;
    m_impl->regexDirty = true;
    NotifyFiltered();
}

void ConsoleFilter::SetCaseSensitive(bool sensitive) {
    m_impl->config.caseSensitive = sensitive;
    m_impl->regexDirty = true;
    NotifyFiltered();
}

bool ConsoleFilter::IsLevelEnabled(LogLevel level) const {
    return m_impl->config.enabledLevels.count(level) > 0;
}

bool ConsoleFilter::MatchesFilter(const ConsoleEntry& entry) const {
    const FilterConfig& cfg = m_impl->config;

    // Level check
    if (!cfg.enabledLevels.count(entry.level)) return false;

    // Age check
    if (cfg.maxAge > 0) {
        uint64_t now = NowMs();
        if (entry.timestamp > 0 && (now - entry.timestamp) > cfg.maxAge) return false;
    }

    // Source filter
    if (!cfg.sourceFilter.empty()) {
        std::string src = cfg.caseSensitive ? entry.source : ToLowerStr(entry.source);
        std::string flt = cfg.caseSensitive ? cfg.sourceFilter : ToLowerStr(cfg.sourceFilter);
        if (src.find(flt) == std::string::npos) return false;
    }

    // Text filter
    if (!cfg.textFilter.empty()) {
        if (cfg.regexEnabled) {
            if (m_impl->regexDirty) m_impl->CompileRegex();
            if (!m_impl->regexValid) return false;
            if (!std::regex_search(entry.text, m_impl->compiledRegex)) return false;
        } else {
            std::string haystack = cfg.caseSensitive ? entry.text : ToLowerStr(entry.text);
            std::string needle   = cfg.caseSensitive ? cfg.textFilter : ToLowerStr(cfg.textFilter);
            if (haystack.find(needle) == std::string::npos) return false;
        }
    }

    return true;
}

bool ConsoleFilter::Export(const std::string& filename) const {
    std::ofstream f(filename);
    if (!f.is_open()) return false;

    for (const auto& e : FilteredEntries()) {
        f << "[" << LevelName(e.level) << "] "
          << "[ts=" << e.timestamp << "] ";
        if (!e.source.empty()) f << e.source << ":" << e.lineNumber << " ";
        f << e.text << "\n";
    }
    return f.good();
}

void ConsoleFilter::OnFiltered(std::function<void()> callback) {
    m_impl->onFiltered = std::move(callback);
}

ConsoleFilterStats ConsoleFilter::GetStats() const {
    ConsoleFilterStats s{};
    s.total = m_impl->all.size();
    for (const auto& e : m_impl->all) {
        s.byLevel[e.level]++;
        if (MatchesFilter(e)) ++s.visible;
        else                  ++s.hidden;
    }
    return s;
}

void ConsoleFilter::NotifyFiltered() {
    if (m_impl->onFiltered) m_impl->onFiltered();
}

} // namespace IDE
