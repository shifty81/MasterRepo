#pragma once
/**
 * @file SamplingProfiler.h
 * @brief Low-overhead CPU sampling profiler that captures call stacks at
 *        configurable intervals.
 *
 * The profiler runs a background sampler thread that signals SIGPROF (POSIX)
 * or uses a high-resolution timer (Windows) to periodically capture the call
 * stack of the main and worker threads.  Results are aggregated into a
 * flat-profile table and a call-tree, exportable to a Flame Graph JSON format.
 *
 * Typical usage:
 * @code
 *   SamplingProfiler prof;
 *   prof.Init();
 *   prof.SetSampleRateHz(1000);
 *   prof.Start();
 *   // ... run code under profiling ...
 *   prof.Stop();
 *   auto report = prof.GetReport();
 *   prof.ExportFlameGraph("Logs/Profiling/flamegraph.json");
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Core {

// ── A sampled stack frame ─────────────────────────────────────────────────────

struct StackFrame {
    uint64_t    address{0};
    std::string symbol;
    std::string file;
    uint32_t    line{0};
};

// ── A single captured sample ──────────────────────────────────────────────────

struct ProfileSample {
    uint32_t                 threadId{0};
    uint64_t                 timestampNs{0};
    std::vector<StackFrame>  frames;  ///< top of stack first
};

// ── Aggregated flat entry ─────────────────────────────────────────────────────

struct FlatProfileEntry {
    std::string symbol;
    std::string file;
    uint32_t    selfHits{0};      ///< samples where this frame was at the top
    uint32_t    totalHits{0};     ///< samples where this frame appeared anywhere
    float       selfPct{0.f};
    float       totalPct{0.f};
};

// ── Profiling report ──────────────────────────────────────────────────────────

struct ProfilingReport {
    uint32_t                      totalSamples{0};
    uint64_t                      durationMs{0};
    uint32_t                      sampleRateHz{0};
    std::vector<FlatProfileEntry> flatProfile;  ///< sorted by totalHits desc
    std::string                   errorMessage;
};

// ── SamplingProfiler ──────────────────────────────────────────────────────────

class SamplingProfiler {
public:
    SamplingProfiler();
    ~SamplingProfiler();

    void Init();
    void Shutdown();

    // ── Configuration ─────────────────────────────────────────────────────────

    void SetSampleRateHz(uint32_t hz);     ///< default 1000 Hz
    void SetMaxSamples(uint32_t count);    ///< 0 = unlimited
    void SetThreadFilter(uint32_t threadId, bool include);

    // ── Control ───────────────────────────────────────────────────────────────

    void Start();
    void Stop();
    void Reset();
    bool IsRunning() const;

    // ── Results ───────────────────────────────────────────────────────────────

    ProfilingReport              GetReport() const;
    std::vector<ProfileSample>   RawSamples() const;
    uint32_t                     SampleCount() const;

    // ── Export ────────────────────────────────────────────────────────────────

    bool ExportFlameGraph(const std::string& jsonPath) const;
    bool ExportFlatCSV(const std::string& csvPath)    const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnSampleCaptured(std::function<void(const ProfileSample&)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Core
