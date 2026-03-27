#pragma once
/**
 * @file MemoryProfiler.h
 * @brief Track, tag, snapshot and diff heap allocations by category/scope.
 *
 * Features:
 *   - BeginFrame() / EndFrame()
 *   - TrackAlloc(ptr, size, category, tag)
 *   - TrackFree(ptr)
 *   - GetTotalAllocated() → size_t
 *   - GetAllocatedByCategory(category) → size_t
 *   - GetPeakAllocated() → size_t
 *   - GetPeakByCategory(category) → size_t
 *   - GetAllocCount() → uint32_t
 *   - TakeSnapshot(snapshotId)
 *   - DiffSnapshots(snapA, snapB, outAdded[], outRemoved[]) → uint32_t
 *   - PrintReport(outText) → uint32_t: line count
 *   - GetCategories(out[]) → uint32_t
 *   - SetOnThreshold(category, bytes, cb): alert when category exceeds limit
 *   - EnableTracking(on)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace Core {

struct AllocRecord {
    void*       ptr{nullptr};
    size_t      size{0};
    std::string category;
    std::string tag;
    uint64_t    frameIndex{0};
};

class MemoryProfiler {
public:
    MemoryProfiler();
    ~MemoryProfiler();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Frame markers
    void BeginFrame();
    void EndFrame  ();

    // Tracking
    void EnableTracking(bool on);
    void TrackAlloc(void* ptr, size_t bytes,
                    const std::string& category = "",
                    const std::string& tag = "");
    void TrackFree (void* ptr);

    // Global stats
    size_t   GetTotalAllocated() const;
    size_t   GetPeakAllocated () const;
    uint32_t GetAllocCount    () const;

    // Per-category
    size_t GetAllocatedByCategory(const std::string& cat) const;
    size_t GetPeakByCategory     (const std::string& cat) const;
    uint32_t GetCategories(std::vector<std::string>& out) const;

    // Snapshots
    void     TakeSnapshot   (uint32_t snapshotId);
    uint32_t DiffSnapshots  (uint32_t snapA, uint32_t snapB,
                             std::vector<AllocRecord>& outAdded,
                             std::vector<AllocRecord>& outRemoved) const;

    // Reporting
    uint32_t PrintReport(std::string& outText) const;

    // Thresholds
    void SetOnThreshold(const std::string& category, size_t bytes,
                        std::function<void(size_t current)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
