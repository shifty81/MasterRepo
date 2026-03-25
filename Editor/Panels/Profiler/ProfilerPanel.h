#pragma once
#include <cstdint>
#include <functional>
#include <numeric>
#include <string>
#include <vector>

namespace Editor {

// ──────────────────────────────────────────────────────────────
// ProfileSample — one recorded CPU timing slice
// ──────────────────────────────────────────────────────────────

struct ProfileSample {
    std::string name;
    uint64_t    duration_us = 0;  // microseconds
    int         depth       = 0;  // call-stack depth
    uint32_t    threadId    = 0;
    uint32_t    frameIndex  = 0;
};

// ──────────────────────────────────────────────────────────────
// FrameStats — per-frame aggregate counters
// ──────────────────────────────────────────────────────────────

struct FrameStats {
    uint32_t frameIndex    = 0;
    float    frameTime_ms  = 0.f;
    float    cpuTime_ms    = 0.f;
    float    gpuTime_ms    = 0.f;
    float    memoryMB      = 0.f;
    uint32_t entityCount   = 0;
    uint32_t drawCalls     = 0;
    uint64_t triangleCount = 0;
};

// ──────────────────────────────────────────────────────────────
// ProfilerPanel — CPU/GPU/memory profiling data store (Phase 10.3)
// ──────────────────────────────────────────────────────────────

class ProfilerPanel {
public:
    explicit ProfilerPanel(uint32_t maxHistory = 256);

    // ── Recording ────────────────────────────────────────────
    void BeginFrame(uint32_t frameIndex);
    void EndFrame(float cpuMs, float gpuMs, float memMB,
                  uint32_t entities, uint32_t draws, uint64_t tris);
    void PushSample(const std::string& name, uint64_t duration_us,
                    int depth, uint32_t threadId);

    // ── Queries ──────────────────────────────────────────────
    const std::vector<FrameStats>& GetFrameHistory() const { return m_frames; }
    const FrameStats*              GetCurrentFrame()  const;
    std::vector<ProfileSample>     GetSamplesForFrame(uint32_t frameIndex) const;

    float GetAverageFPS(int lastN = 60)    const;
    float GetAverageCPUMs(int lastN = 60)  const;
    float GetAverageGPUMs(int lastN = 60)  const;
    float GetPeakMemoryMB()                const;

    const FrameStats* GetSlowestFrame() const;

    // ── Control ──────────────────────────────────────────────
    void     Clear();
    void     SetMaxHistory(uint32_t n);
    uint32_t MaxHistory() const { return m_maxHistory; }

    bool IsPaused() const    { return m_paused; }
    void SetPaused(bool p)   { m_paused = p; }

    // ── PL-03: Live data source ──────────────────────────────
    /// Query function called by Tick() each frame to fill live stats.
    struct LiveStats {
        float    cpuMs     = 0.f;
        float    gpuMs     = 0.f;
        float    memoryMB  = 0.f;
        uint32_t entities  = 0;
        uint32_t drawCalls = 0;
        uint64_t triangles = 0;
    };
    using LiveDataFn = std::function<LiveStats()>;
    void SetLiveDataSource(LiveDataFn fn)  { m_liveDataFn = std::move(fn); }

    /// Call once per game frame to automatically record live stats.
    void Tick(float dt);

    bool HasLiveData() const { return static_cast<bool>(m_liveDataFn); }

private:
    uint32_t                 m_maxHistory;
    bool                     m_paused       = false;
    FrameStats               m_currentFrame;
    bool                     m_frameOpen    = false;
    std::vector<FrameStats>  m_frames;      // circular, capped at m_maxHistory
    std::vector<ProfileSample> m_samples;  // all samples across stored frames
    LiveDataFn               m_liveDataFn;
    uint32_t                 m_frameCounter = 0;
    float                    m_tickAccum    = 0.f;
};

} // namespace Editor
