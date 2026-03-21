#pragma once
// RenderingOptimizer — AI-driven rendering performance advisor
//
// Analyses the active RenderPipeline configuration and per-frame profiling
// data, then produces prioritised optimisation suggestions that can be applied
// automatically or reviewed by the developer.
//
// All suggestions are rule-based and offline — no LLM call required for
// standard tips.  An optional LLM hook can generate free-form explanations.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace AI {

// ─────────────────────────────────────────────────────────────────────────────
// Frame metrics (supplied by the calling system each frame)
// ─────────────────────────────────────────────────────────────────────────────

struct FrameMetrics {
    float    frameTimeMs        = 0.f;
    float    gpuTimeMs          = 0.f;
    float    cpuTimeMs          = 0.f;
    int      drawCalls          = 0;
    int      triangleCount      = 0;
    int      particleCount      = 0;
    int      shadowCascades     = 0;
    int      reflectionProbes   = 0;
    int      lodGroupsActive    = 0;
    float    textureMemoryMB    = 0.f;
    float    vertexMemoryMB     = 0.f;
    bool     giEnabled          = false;
    bool     volumetricsEnabled = false;
    bool     rayTracingEnabled  = false;
    float    targetFPS          = 60.f;
};

// ─────────────────────────────────────────────────────────────────────────────
// Suggestion
// ─────────────────────────────────────────────────────────────────────────────

enum class OptimPriority { Low, Medium, High, Critical };

enum class OptimCategory {
    DrawCall,
    Memory,
    Particle,
    Shadow,
    GI,
    LOD,
    PostProcess,
    Shader,
    General,
};

struct OptimSuggestion {
    OptimPriority priority     = OptimPriority::Medium;
    OptimCategory category     = OptimCategory::General;
    std::string   title;        // short headline
    std::string   description;  // detailed explanation
    std::string   action;       // machine-readable key, e.g. "reduce_draw_calls"
    float         estimatedGainMs = 0.f; // expected frame time saving
    bool          autoApplicable  = false; // can be auto-applied?
};

// ─────────────────────────────────────────────────────────────────────────────
// RenderingOptimizerConfig
// ─────────────────────────────────────────────────────────────────────────────

struct RenderingOptimizerConfig {
    float   targetFPS            = 60.f;
    int     maxDrawCallsPerFrame = 2000;
    int     maxParticlesPerFrame = 50000;
    float   maxTextureMemoryMB   = 512.f;
    // Window (frames) for rolling-average smoothing
    int     smoothingWindow      = 30;
    // Optional LLM hook: receives suggestion and returns richer explanation
    std::function<std::string(const OptimSuggestion&)> llmExplainFn;
};

// ─────────────────────────────────────────────────────────────────────────────
// RenderingOptimizer
// ─────────────────────────────────────────────────────────────────────────────

class RenderingOptimizer {
public:
    explicit RenderingOptimizer(RenderingOptimizerConfig cfg = {});

    // ── Per-frame data ingestion ──────────────────────────────────────────────

    /// Feed new frame metrics; call once per rendered frame.
    void FeedFrame(const FrameMetrics& metrics);

    // ── Analysis ─────────────────────────────────────────────────────────────

    /// Run the rule engine on recent frame data.
    /// Returns a list of suggestions sorted by priority (Critical first).
    std::vector<OptimSuggestion> Analyse() const;

    /// Return only suggestions that can be automatically applied.
    std::vector<OptimSuggestion> GetAutoSuggestions() const;

    // ── Application ──────────────────────────────────────────────────────────

    /// Apply all auto-applicable suggestions.
    /// Returns a list of action keys that were applied.
    std::vector<std::string> AutoApply();

    // ── Reporting ────────────────────────────────────────────────────────────

    /// Produce a Markdown performance report.
    std::string BuildReport() const;

    /// Write report to a file.
    bool WriteReport(const std::string& path) const;

    // ── Stats ────────────────────────────────────────────────────────────────

    float AverageFrameTimeMs() const;
    float AverageGPUTimeMs()   const;
    float WorstFrameTimeMs()   const;
    int   FrameCount()         const;

    void Reset();

private:
    RenderingOptimizerConfig     m_cfg;
    std::vector<FrameMetrics>    m_history;

    FrameMetrics ComputeAverage() const;
    void CheckDrawCalls(const FrameMetrics& avg,
                        std::vector<OptimSuggestion>& out) const;
    void CheckMemory(const FrameMetrics& avg,
                     std::vector<OptimSuggestion>& out) const;
    void CheckParticles(const FrameMetrics& avg,
                        std::vector<OptimSuggestion>& out) const;
    void CheckGI(const FrameMetrics& avg,
                 std::vector<OptimSuggestion>& out) const;
    void CheckLOD(const FrameMetrics& avg,
                  std::vector<OptimSuggestion>& out) const;
    void CheckVolumetrics(const FrameMetrics& avg,
                          std::vector<OptimSuggestion>& out) const;
};

} // namespace AI
