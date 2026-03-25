#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace IDE {

// ── Enumerations ─────────────────────────────────────────────────────────────

enum class LogLevel : uint8_t {
    Debug   = 0,
    Info    = 1,
    Warning = 2,
    Error   = 3,
    Fatal   = 4
};

// ── Data types ────────────────────────────────────────────────────────────────

struct ConsoleEntry {
    std::string text;
    LogLevel    level{LogLevel::Info};
    uint64_t    timestamp{0};   // milliseconds since epoch
    std::string source;
    int         lineNumber{0};
};

struct FilterConfig {
    std::set<LogLevel> enabledLevels{
        LogLevel::Debug, LogLevel::Info,
        LogLevel::Warning, LogLevel::Error, LogLevel::Fatal
    };
    std::string textFilter;
    bool        regexEnabled{false};
    bool        caseSensitive{false};
    std::string sourceFilter;    // substring match on entry.source
    uint64_t    maxAge{0};       // 0 = no age limit (ms)
};

struct ConsoleFilterStats {
    uint64_t                    total{0};
    uint64_t                    visible{0};
    uint64_t                    hidden{0};
    std::map<LogLevel, uint64_t> byLevel;
};

// ── ConsoleFilter ─────────────────────────────────────────────────────────────

class ConsoleFilter {
public:
    ConsoleFilter();
    ~ConsoleFilter();

    // Entry management
    void AddEntry(const ConsoleEntry& entry);
    void Clear();

    // Configuration
    void                SetConfig(const FilterConfig& config);
    const FilterConfig& GetConfig() const;

    // Filtered / unfiltered views
    std::vector<ConsoleEntry> FilteredEntries() const;
    const std::vector<ConsoleEntry>& AllEntries() const;
    size_t EntryCount()    const;
    size_t FilteredCount() const;

    // Convenience setters
    void SetTextFilter(const std::string& text);
    void SetLevelFilter(LogLevel level, bool enabled);
    void EnableRegex(bool enable);
    void SetCaseSensitive(bool sensitive);

    // Predicates
    bool IsLevelEnabled(LogLevel level) const;
    bool MatchesFilter(const ConsoleEntry& entry) const;

    // Export
    bool Export(const std::string& filename) const;

    // Callback — fires whenever the active filter changes
    void OnFiltered(std::function<void()> callback);

    // Stats
    ConsoleFilterStats GetStats() const;

private:
    void NotifyFiltered();

    struct Impl;
    Impl* m_impl;
};

} // namespace IDE
