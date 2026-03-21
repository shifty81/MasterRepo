#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace Tools {

/// A single frame/sample captured by the profiler.
struct ProfileSample {
    std::string name;           ///< Subsystem or scope label
    uint64_t    startUs{0};     ///< Start time in microseconds (since profiler start)
    uint64_t    durationUs{0};  ///< Duration in microseconds
    int         depth{0};       ///< Nesting level (for call-tree display)
    uint32_t    frameIndex{0};
};

/// Aggregated stats for a named scope.
struct ScopeStats {
    std::string name;
    uint64_t    totalUs{0};
    uint64_t    minUs{UINT64_MAX};
    uint64_t    maxUs{0};
    uint64_t    avgUs{0};
    uint32_t    callCount{0};
    uint32_t    frameCount{0};
};

using ProfileCallback = std::function<void(const ProfileSample&)>;

/// PerformanceProfiler — frame-time and subsystem profiler.
///
/// Scopes are opened with BeginScope(name) / EndScope() (RAII helper
/// ProfileScope wraps these).  At the end of each frame EndFrame()
/// archives all samples for the frame and updates running averages.
/// Data is queryable by the AnalyticsDashboard or the editor profiler panel.
class PerformanceProfiler {
public:
    PerformanceProfiler();
    ~PerformanceProfiler();

    // ── lifecycle ─────────────────────────────────────────────
    void Start();
    void Stop();
    bool IsRunning() const;

    // ── frame control ─────────────────────────────────────────
    /// Call at the start of each engine tick/frame.
    void BeginFrame();
    /// Call at the end; archives samples and increments frame counter.
    void EndFrame();
    uint32_t CurrentFrame() const;

    // ── scope recording ───────────────────────────────────────
    void BeginScope(const std::string& name);
    void EndScope();

    // ── stats query ───────────────────────────────────────────
    /// All samples from the most recent completed frame.
    std::vector<ProfileSample> GetLastFrameSamples() const;
    /// Aggregated stats for a named scope over all frames.
    ScopeStats  GetStats(const std::string& name) const;
    /// Aggregated stats for all tracked scopes.
    std::vector<ScopeStats> GetAllStats() const;
    /// Average frame time over the last N frames.
    double      AvgFrameTimeMs(uint32_t lastN = 60) const;
    void        ResetStats();

    // ── callbacks ─────────────────────────────────────────────
    void OnSample(ProfileCallback cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

/// RAII profiler scope helper.
struct ProfileScope {
    PerformanceProfiler& profiler;
    explicit ProfileScope(PerformanceProfiler& p, const std::string& name)
        : profiler(p) { profiler.BeginScope(name); }
    ~ProfileScope() { profiler.EndScope(); }
};

} // namespace Tools
