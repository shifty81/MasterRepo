#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace AI {

/// A snapshot of current resource usage.
struct ResourceSnapshot {
    float    cpuUsagePercent{0.0f};  ///< 0 – 100
    uint64_t ramUsedBytes{0};
    uint64_t ramTotalBytes{0};
    float    gpuUsagePercent{0.0f};  ///< 0 – 100 (stub: 0 if GPU unavailable)
    uint64_t gpuVramUsedBytes{0};
    float    sessionElapsedSec{0.0f};
};

/// Threshold configuration.
struct ResourceThresholds {
    float    maxCpuPercent{90.0f};
    float    maxRamPercent{85.0f};
    float    maxGpuPercent{90.0f};
    float    maxSessionSec{3600.0f};  ///< 1-hour AI session limit
};

enum class ResourceAlert { CPU, RAM, GPU, SessionTime };

using SnapshotCallback = std::function<void(const ResourceSnapshot&)>;
using AlertCallback    = std::function<void(ResourceAlert, const ResourceSnapshot&)>;

/// ResourceMonitor — tracks CPU/RAM/GPU usage during long AI sessions.
///
/// Polls system resources at a configurable interval.  When thresholds
/// are exceeded the alert callback fires so the AI orchestrator can
/// throttle, pause, or checkpoint its work.
class ResourceMonitor {
public:
    ResourceMonitor();
    ~ResourceMonitor();

    // ── lifecycle ──────────────────────────────────────────────
    /// Begin monitoring.  Resets session timer.
    void Start();
    /// Stop monitoring.
    void Stop();
    bool IsRunning() const;

    // ── configuration ─────────────────────────────────────────
    void SetThresholds(const ResourceThresholds& t);
    ResourceThresholds GetThresholds() const;
    /// Poll interval in milliseconds (default 1000).
    void SetPollIntervalMs(uint32_t ms);

    // ── polling ───────────────────────────────────────────────
    /// Sample resources now (also called internally on interval).
    ResourceSnapshot Sample();
    /// Most-recent snapshot (without re-sampling).
    ResourceSnapshot GetLast() const;

    // ── callbacks ─────────────────────────────────────────────
    void OnSample(SnapshotCallback cb);
    void OnAlert(AlertCallback cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace AI
