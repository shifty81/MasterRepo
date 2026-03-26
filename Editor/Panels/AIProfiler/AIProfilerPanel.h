#pragma once
/**
 * @file AIProfilerPanel.h
 * @brief Editor panel for profiling AI and PCG subsystem performance.
 *
 * Displays:
 *   - Per-frame AI + PCG CPU time
 *   - Request queue depth and throughput
 *   - Active reasoning traces (collapsed list)
 *   - Anomaly alerts fired this session
 *   - Memory usage of AI embeddings and session memory
 *
 * Follows the Atlas Editor panel convention: Draw() is called each frame by
 * the EditorRenderer after the layout pass.
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Editor {

// ── A single sampled frame metric for the profiler graph ──────────────────────

struct AIProfileSample {
    float    aiCpuMs{0.0f};  ///< AI subsystem time this frame
    float    pcgCpuMs{0.0f}; ///< PCG subsystem time this frame
    uint32_t queueDepth{0};
    uint64_t timestampMs{0};
};

// ── AIProfilerPanel ───────────────────────────────────────────────────────────

class AIProfilerPanel {
public:
    AIProfilerPanel();
    ~AIProfilerPanel();

    void Init();
    void Shutdown();

    // ── Per-frame interface ───────────────────────────────────────────────────

    /// Feed a new sample at the start of each frame.
    void PushSample(float aiCpuMs, float pcgCpuMs, uint32_t queueDepth = 0);

    /// Draw the panel contents (called by EditorRenderer).
    void Draw();

    // ── Panel visibility ──────────────────────────────────────────────────────

    bool IsVisible()                  const;
    void SetVisible(bool visible);
    void ToggleVisible();

    // ── Configuration ─────────────────────────────────────────────────────────

    /// Number of frames to keep in the rolling history graph.
    void SetHistoryDepth(uint32_t frames);

    /// Show anomaly alerts inline in the panel.
    void SetShowAlerts(bool show);

    // ── Data access ───────────────────────────────────────────────────────────

    std::vector<AIProfileSample> Samples()          const;
    float                        PeakAICpuMs()      const;
    float                        AvgAICpuMs()       const;
    float                        PeakPCGCpuMs()     const;
    float                        AvgPCGCpuMs()      const;

    void Clear();

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Editor
