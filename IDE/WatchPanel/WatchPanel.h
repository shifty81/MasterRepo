#pragma once
/**
 * @file WatchPanel.h
 * @brief IDE variable watch list: add expressions, snapshot values, track deltas.
 *
 * WatchPanel stores a list of named watch expressions. At each Snapshot() call,
 * the current string value of each watch is recorded, a delta vs. the previous
 * snapshot is computed, and an optional change callback is fired.
 *
 * Values are provided via a user-supplied EvalCb; the WatchPanel has no dependency
 * on any scripting engine — it is purely a data-management layer.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace IDE {

// ── Watch entry ────────────────────────────────────────────────────────────
struct WatchEntry {
    uint32_t    id{0};
    std::string expression;
    std::string currentValue;
    std::string previousValue;
    bool        changed{false};    ///< True if value differed from previous snapshot
    bool        enabled{true};
    uint32_t    snapshotCount{0};
    std::string note;              ///< Optional user annotation
};

// ── Eval callback ─────────────────────────────────────────────────────────
/// Caller supplies an evaluator that maps expression string → current value string.
using EvalCb = std::function<std::string(const std::string& expression)>;

// ── Change callback ───────────────────────────────────────────────────────
using WatchChangeCb = std::function<void(const WatchEntry&)>;

// ── Panel ─────────────────────────────────────────────────────────────────
class WatchPanel {
public:
    WatchPanel();
    ~WatchPanel();

    // ── watch management ──────────────────────────────────────
    uint32_t AddWatch(const std::string& expression,
                      const std::string& note = "");
    bool     RemoveWatch(uint32_t id);
    bool     RemoveWatchByExpr(const std::string& expression);
    void     ClearAll();
    bool     SetEnabled(uint32_t id, bool enabled);
    bool     SetNote(uint32_t id, const std::string& note);

    // ── queries ───────────────────────────────────────────────
    const WatchEntry* GetEntry(uint32_t id) const;
    std::vector<WatchEntry> GetAll() const;
    std::vector<WatchEntry> GetChanged() const;  ///< Entries that changed in last snapshot
    size_t Count() const;
    bool   HasChanges() const;

    // ── snapshot ──────────────────────────────────────────────
    /// Evaluate all enabled watches using @p eval; update currentValue and changed flag.
    void Snapshot(EvalCb eval);
    /// Snapshot count across all entries (for detecting stale watches).
    uint32_t SnapshotRound() const;

    // ── callbacks ─────────────────────────────────────────────
    void OnChange(WatchChangeCb cb);

    // ── export ────────────────────────────────────────────────
    std::string ExportText() const;
    std::string ExportCSV() const;

    // ── history ───────────────────────────────────────────────
    /// Store last @p maxHistory values per watch.
    void SetHistoryDepth(uint32_t maxHistory);
    std::vector<std::string> GetHistory(uint32_t id) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace IDE
