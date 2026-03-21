#pragma once
/**
 * @file MemoryProfiler.h
 * @brief Memory allocation tracking, high-water marks, and snapshot diffing.
 *
 * MemoryProfiler provides:
 *   - AllocationRecord: per-allocation metadata (tag, size, callsite)
 *   - Scoped tracking: track()/untrack() per logical category tag
 *   - MemorySnapshot: point-in-time capture of live allocation stats
 *   - Diff: compare two snapshots to find leaks or spikes
 *   - Report: human-readable allocation summary (text / CSV)
 *   - High-water mark: per-tag peak recorded since last reset
 *
 * This is a CPU-side instrumentation layer; no OS allocation hooks are used.
 * Callers explicitly record alloc/free events (or use the RAII scope helper).
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Tools {

// ── Record ────────────────────────────────────────────────────────────────
struct AllocationRecord {
    uint64_t    id{0};
    std::string tag;          ///< Logical category (e.g. "Textures", "Mesh")
    size_t      bytes{0};
    std::string callsite;     ///< Optional file:line hint
    uint64_t    timestampMs{0};
    bool        freed{false};
};

// ── Per-tag stats ─────────────────────────────────────────────────────────
struct TagStats {
    std::string tag;
    size_t      liveBytes{0};
    size_t      liveCount{0};
    size_t      peakBytes{0};    ///< High-water mark since last Reset
    size_t      totalAllocated{0};
    size_t      totalFreed{0};
};

// ── Snapshot ─────────────────────────────────────────────────────────────
struct MemorySnapshot {
    uint64_t                              timestampMs{0};
    std::string                           label;
    std::unordered_map<std::string, TagStats> tagStats;
    size_t                                totalLiveBytes{0};
    size_t                                totalAllocations{0};
};

// ── Diff ──────────────────────────────────────────────────────────────────
struct SnapshotDiff {
    std::string                           fromLabel;
    std::string                           toLabel;
    std::unordered_map<std::string, int64_t> deltaBytes;  ///< signed change per tag
    int64_t                               totalDeltaBytes{0};
};

SnapshotDiff DiffSnapshots(const MemorySnapshot& from,
                            const MemorySnapshot& to);

// ── RAII scope tracker ────────────────────────────────────────────────────
// Forward declare; actual usage requires a MemoryProfiler* pointer
class MemoryProfiler;

struct ScopeTracker {
    ScopeTracker(MemoryProfiler* mp, const std::string& tag,
                 size_t bytes, const std::string& callsite = "");
    ~ScopeTracker();

    ScopeTracker(const ScopeTracker&) = delete;
    ScopeTracker& operator=(const ScopeTracker&) = delete;

private:
    MemoryProfiler* m_mp{nullptr};
    uint64_t        m_id{0};
};

// ── Profiler ──────────────────────────────────────────────────────────────
class MemoryProfiler {
public:
    MemoryProfiler();
    ~MemoryProfiler();

    // ── tracking ─────────────────────────────────────────────
    uint64_t Track(const std::string& tag, size_t bytes,
                   const std::string& callsite = "");
    void     Untrack(uint64_t id);

    // ── query ─────────────────────────────────────────────────
    size_t LiveBytes(const std::string& tag = "") const;
    size_t LiveCount(const std::string& tag = "") const;
    size_t PeakBytes(const std::string& tag = "") const;

    TagStats       GetTagStats(const std::string& tag) const;
    std::vector<TagStats> AllTagStats() const;

    std::vector<std::string> Tags() const;
    size_t TagCount() const;

    // ── snapshots ─────────────────────────────────────────────
    MemorySnapshot Snapshot(const std::string& label = "") const;

    // ── report ────────────────────────────────────────────────
    std::string Report() const;             ///< Human-readable summary
    std::string ReportCSV() const;          ///< CSV with tag,live,peak

    // ── reset ─────────────────────────────────────────────────
    void ResetPeaks();   ///< Clear high-water marks only
    void ResetAll();     ///< Clear everything

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Tools
