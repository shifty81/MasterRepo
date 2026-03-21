#include "Tools/MemoryProfiler/MemoryProfiler.h"
#include <chrono>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace Tools {

static uint64_t mpNowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()).count());
}

// ── SnapshotDiff ──────────────────────────────────────────────────────────
SnapshotDiff DiffSnapshots(const MemorySnapshot& from,
                            const MemorySnapshot& to)
{
    SnapshotDiff diff;
    diff.fromLabel = from.label;
    diff.toLabel   = to.label;

    for (const auto& [tag, ts] : to.tagStats) {
        auto it = from.tagStats.find(tag);
        int64_t prev = it != from.tagStats.end() ? static_cast<int64_t>(it->second.liveBytes) : 0;
        int64_t delta = static_cast<int64_t>(ts.liveBytes) - prev;
        diff.deltaBytes[tag] = delta;
        diff.totalDeltaBytes += delta;
    }
    // Tags in from but not in to (freed)
    for (const auto& [tag, fs] : from.tagStats) {
        if (!to.tagStats.count(tag)) {
            diff.deltaBytes[tag] = -static_cast<int64_t>(fs.liveBytes);
            diff.totalDeltaBytes -= static_cast<int64_t>(fs.liveBytes);
        }
    }
    return diff;
}

// ── ScopeTracker ──────────────────────────────────────────────────────────
ScopeTracker::ScopeTracker(MemoryProfiler* mp,
                             const std::string& tag,
                             size_t bytes,
                             const std::string& callsite)
    : m_mp(mp), m_id(0)
{
    if (mp) m_id = mp->Track(tag, bytes, callsite);
}
ScopeTracker::~ScopeTracker() {
    if (m_mp && m_id) m_mp->Untrack(m_id);
}

// ── Impl ─────────────────────────────────────────────────────────────────
struct MemoryProfiler::Impl {
    std::unordered_map<uint64_t, AllocationRecord> records;
    std::unordered_map<std::string, TagStats>       tags;
    uint64_t nextId{1};
};

MemoryProfiler::MemoryProfiler() : m_impl(new Impl()) {}
MemoryProfiler::~MemoryProfiler() { delete m_impl; }

uint64_t MemoryProfiler::Track(const std::string& tag,
                                 size_t bytes,
                                 const std::string& callsite)
{
    AllocationRecord rec;
    rec.id          = m_impl->nextId++;
    rec.tag         = tag;
    rec.bytes       = bytes;
    rec.callsite    = callsite;
    rec.timestampMs = mpNowMs();
    m_impl->records[rec.id] = rec;

    auto& ts = m_impl->tags[tag];
    ts.tag              = tag;
    ts.liveBytes       += bytes;
    ts.liveCount       += 1;
    ts.totalAllocated  += bytes;
    if (ts.liveBytes > ts.peakBytes) ts.peakBytes = ts.liveBytes;

    return rec.id;
}

void MemoryProfiler::Untrack(uint64_t id) {
    auto it = m_impl->records.find(id);
    if (it == m_impl->records.end() || it->second.freed) return;
    it->second.freed = true;
    const auto& rec = it->second;
    auto& ts = m_impl->tags[rec.tag];
    if (ts.liveBytes >= rec.bytes) ts.liveBytes -= rec.bytes;
    if (ts.liveCount > 0)          ts.liveCount  -= 1;
    ts.totalFreed += rec.bytes;
}

size_t MemoryProfiler::LiveBytes(const std::string& tag) const {
    if (tag.empty()) {
        size_t total = 0;
        for (const auto& [_,ts] : m_impl->tags) total += ts.liveBytes;
        return total;
    }
    auto it = m_impl->tags.find(tag);
    return it != m_impl->tags.end() ? it->second.liveBytes : 0;
}

size_t MemoryProfiler::LiveCount(const std::string& tag) const {
    if (tag.empty()) {
        size_t total = 0;
        for (const auto& [_,ts] : m_impl->tags) total += ts.liveCount;
        return total;
    }
    auto it = m_impl->tags.find(tag);
    return it != m_impl->tags.end() ? it->second.liveCount : 0;
}

size_t MemoryProfiler::PeakBytes(const std::string& tag) const {
    if (tag.empty()) {
        size_t peak = 0;
        for (const auto& [_,ts] : m_impl->tags) peak = std::max(peak, ts.peakBytes);
        return peak;
    }
    auto it = m_impl->tags.find(tag);
    return it != m_impl->tags.end() ? it->second.peakBytes : 0;
}

TagStats MemoryProfiler::GetTagStats(const std::string& tag) const {
    auto it = m_impl->tags.find(tag);
    return it != m_impl->tags.end() ? it->second : TagStats{tag,0,0,0,0,0};
}

std::vector<TagStats> MemoryProfiler::AllTagStats() const {
    std::vector<TagStats> out;
    out.reserve(m_impl->tags.size());
    for (const auto& [_,ts] : m_impl->tags) out.push_back(ts);
    std::sort(out.begin(), out.end(),
        [](const TagStats& a, const TagStats& b){ return a.liveBytes > b.liveBytes; });
    return out;
}

std::vector<std::string> MemoryProfiler::Tags() const {
    std::vector<std::string> out;
    for (const auto& [k,_] : m_impl->tags) out.push_back(k);
    std::sort(out.begin(), out.end());
    return out;
}
size_t MemoryProfiler::TagCount() const { return m_impl->tags.size(); }

MemorySnapshot MemoryProfiler::Snapshot(const std::string& label) const {
    MemorySnapshot snap;
    snap.timestampMs = mpNowMs();
    snap.label       = label;
    for (const auto& [tag, ts] : m_impl->tags) {
        snap.tagStats[tag]    = ts;
        snap.totalLiveBytes  += ts.liveBytes;
        snap.totalAllocations += ts.liveCount;
    }
    return snap;
}

std::string MemoryProfiler::Report() const {
    std::ostringstream ss;
    ss << "=== MemoryProfiler Report ===\n";
    ss << std::left << std::setw(20) << "Tag"
       << std::right << std::setw(12) << "Live(B)"
       << std::setw(8) << "Count"
       << std::setw(12) << "Peak(B)" << "\n";
    ss << std::string(52, '-') << "\n";
    for (const auto& ts : AllTagStats()) {
        ss << std::left  << std::setw(20) << ts.tag
           << std::right << std::setw(12) << ts.liveBytes
           << std::setw(8) << ts.liveCount
           << std::setw(12) << ts.peakBytes << "\n";
    }
    ss << std::string(52, '-') << "\n";
    ss << std::left << std::setw(20) << "TOTAL"
       << std::right << std::setw(12) << LiveBytes() << "\n";
    return ss.str();
}

std::string MemoryProfiler::ReportCSV() const {
    std::ostringstream ss;
    ss << "tag,live_bytes,live_count,peak_bytes,total_allocated,total_freed\n";
    for (const auto& ts : AllTagStats()) {
        ss << ts.tag << "," << ts.liveBytes << "," << ts.liveCount << ","
           << ts.peakBytes << "," << ts.totalAllocated << "," << ts.totalFreed << "\n";
    }
    return ss.str();
}

void MemoryProfiler::ResetPeaks() {
    for (auto& [_,ts] : m_impl->tags) ts.peakBytes = ts.liveBytes;
}

void MemoryProfiler::ResetAll() {
    m_impl->records.clear();
    m_impl->tags.clear();
    m_impl->nextId = 1;
}

} // namespace Tools
