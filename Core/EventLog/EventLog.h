#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace Core {

/// Severity of a log entry.
enum class EventSeverity { Debug, Info, Warning, Error, Critical };

/// A single log entry in the event log.
struct EventEntry {
    uint64_t        sequenceId{0};  ///< Monotonic counter
    uint64_t        timestampMs{0}; ///< Milliseconds since epoch (or boot)
    std::string     source;         ///< Subsystem that emitted this entry
    std::string     type;           ///< Event type tag (matches EventBus event types)
    std::string     message;
    EventSeverity   severity{EventSeverity::Info};
    std::string     extra;          ///< JSON-encoded extra payload (optional)
};

using EntryCallback = std::function<void(const EventEntry&)>;

/// EventLog — persistent audit / event log.
///
/// Subsystems and the EventBus feed entries here.  The log is queryable
/// by source, type, and severity, and can be flushed to disk for AI
/// training-data collection or post-mortem debugging.  A rolling-window
/// cap prevents unbounded memory use.
class EventLog {
public:
    EventLog();
    ~EventLog();

    // ── configuration ─────────────────────────────────────────
    /// Maximum number of entries kept in memory (oldest dropped; default 10 000).
    void SetCapacity(size_t maxEntries);
    size_t GetCapacity() const;
    /// Optional path prefix for flushing to disk ("<prefix>_<timestamp>.jsonl").
    void SetFlushPath(const std::string& pathPrefix);

    // ── writing ───────────────────────────────────────────────
    /// Append a fully constructed entry.
    void Append(const EventEntry& entry);
    /// Convenience: create and append an entry.
    void Log(const std::string& source, const std::string& type,
             const std::string& message,
             EventSeverity severity = EventSeverity::Info,
             const std::string& extra = "");
    void Clear();

    // ── reading ───────────────────────────────────────────────
    size_t Count() const;
    /// All entries (oldest first).
    std::vector<EventEntry> GetAll()    const;
    /// Filter by minimum severity.
    std::vector<EventEntry> GetBySeverity(EventSeverity minSeverity) const;
    /// Filter by source subsystem name.
    std::vector<EventEntry> GetBySource(const std::string& source) const;
    /// Filter by event type tag.
    std::vector<EventEntry> GetByType(const std::string& type)   const;
    /// Last N entries.
    std::vector<EventEntry> GetTail(size_t n) const;

    // ── persistence ───────────────────────────────────────────
    /// Write all in-memory entries to disk (JSONL format) and clear.
    bool FlushToDisk();

    // ── callbacks ─────────────────────────────────────────────
    /// Fired for every new entry appended.
    void OnEntry(EntryCallback cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
