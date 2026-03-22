#pragma once
/**
 * @file GPUProfiler.h
 * @brief GPU frame profiling with named zone tracking and frame history.
 *
 * GPUProfiler records CPU-side timing for named GPU work zones:
 *   - GPUZone: label, startMs, durationMs, depth (nesting level).
 *   - GPUFrame: vector of zones, totalGPUTimeMs, frameIndex.
 *   - BeginFrame(): start a new frame capture.
 *   - EndFrame(): finalise current frame; returns the completed GPUFrame.
 *   - PushZone(label): begin a named timing zone; supports nesting.
 *   - PopZone(): end the innermost open zone.
 *   - ScopedZone RAII helper: auto-pops on destruction.
 *   - GetLastFrame(): the most recently completed frame.
 *   - GetFrameHistory(n): last N frames (up to kMaxHistory = 128).
 *   - AverageFrameTimeMs(): rolling average over the history window.
 *   - PeakFrameTimeMs(): maximum over the history window.
 *   - Reset(): clear history and counters.
 *   - GPUProfilerStats: framesRecorded, totalZones, avgFrameMs, peakFrameMs.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace Tools {

// ── GPU zone ──────────────────────────────────────────────────────────────
struct GPUZone {
    std::string label;
    double      startMs{0.0};
    double      durationMs{0.0};
    uint32_t    depth{0};       ///< Nesting depth (0 = top level)
};

// ── GPU frame ─────────────────────────────────────────────────────────────
struct GPUFrame {
    uint64_t             frameIndex{0};
    std::vector<GPUZone> zones;
    double               totalGPUTimeMs{0.0}; ///< Sum of top-level zone durations
};

// ── Stats ─────────────────────────────────────────────────────────────────
struct GPUProfilerStats {
    uint64_t framesRecorded{0};
    uint64_t totalZones{0};
    double   avgFrameMs{0.0};
    double   peakFrameMs{0.0};
};

// ── GPUProfiler ───────────────────────────────────────────────────────────
class GPUProfiler {
public:
    static constexpr size_t kMaxHistory = 128;

    GPUProfiler();
    ~GPUProfiler();

    GPUProfiler(const GPUProfiler&) = delete;
    GPUProfiler& operator=(const GPUProfiler&) = delete;

    // ── frame control ─────────────────────────────────────────
    void     BeginFrame();
    GPUFrame EndFrame();

    // ── zone control ──────────────────────────────────────────
    void PushZone(const std::string& label);
    void PopZone();

    // ── RAII helper ───────────────────────────────────────────
    struct ScopedZone {
        GPUProfiler& profiler;
        explicit ScopedZone(GPUProfiler& p, const std::string& label) : profiler(p) { p.PushZone(label); }
        ~ScopedZone() { profiler.PopZone(); }
    };

    // ── history queries ───────────────────────────────────────
    const GPUFrame*             GetLastFrame() const;
    std::vector<const GPUFrame*> GetFrameHistory(size_t n) const;

    double AverageFrameTimeMs() const;
    double PeakFrameTimeMs()    const;

    // ── reset ─────────────────────────────────────────────────
    void Reset();

    // ── stats ─────────────────────────────────────────────────
    GPUProfilerStats Stats() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Tools
