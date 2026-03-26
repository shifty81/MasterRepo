#pragma once
/**
 * @file ReasoningVisualizer.h
 * @brief Visualise AI decision reasoning — surfaces "why" an AI suggestion
 *        was generated, with confidence scores, input evidence, and alternative
 *        paths that were considered.
 *
 * Typical usage:
 * @code
 *   ReasoningVisualizer rv;
 *   rv.Init();
 *   uint64_t id = rv.BeginTrace("generate_mesh_lod");
 *   rv.AddEvidence(id, "poly_count", "48000 polys, threshold=10000");
 *   rv.AddDecisionStep(id, "Apply LOD reduction", 0.92f);
 *   rv.Finalize(id, "LOD level 2 generated");
 *   rv.RenderOverlay(id);   // draw in editor viewport
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace AI {

// ── Evidence item attached to a reasoning trace ───────────────────────────────

struct ReasoningEvidence {
    std::string key;         ///< e.g. "poly_count", "player_level"
    std::string value;       ///< human-readable value string
    float       weight{1.0f};///< relative importance in the decision
};

// ── A single step in the decision chain ──────────────────────────────────────

struct DecisionStep {
    std::string description;     ///< what the AI decided at this step
    float       confidence{0.0f};///< 0–1
    std::string alternativeConsidered; ///< best rejected alternative (optional)
    float       alternativeScore{0.0f};
};

// ── A complete reasoning trace for one AI action ──────────────────────────────

struct ReasoningTrace {
    uint64_t                 id{0};
    std::string              actionTag;    ///< e.g. "generate_mesh_lod"
    std::vector<ReasoningEvidence> evidence;
    std::vector<DecisionStep>     steps;
    std::string              conclusion;   ///< final human-readable outcome
    bool                     finalized{false};
    uint64_t                 timestampMs{0};
};

// ── ReasoningVisualizer ───────────────────────────────────────────────────────

class ReasoningVisualizer {
public:
    ReasoningVisualizer();
    ~ReasoningVisualizer();

    void Init();
    void Shutdown();

    // ── Trace lifecycle ───────────────────────────────────────────────────────

    /// Start a new reasoning trace for the given action tag.
    /// @returns trace id used in all subsequent calls.
    uint64_t BeginTrace(const std::string& actionTag);

    /// Attach a piece of evidence to a trace.
    void AddEvidence(uint64_t traceId, const std::string& key,
                     const std::string& value, float weight = 1.0f);

    /// Record one decision step with confidence and optional alternative.
    void AddDecisionStep(uint64_t traceId,
                         const std::string& description,
                         float confidence,
                         const std::string& alternative = "",
                         float alternativeScore = 0.0f);

    /// Mark the trace as complete with a conclusion string.
    void Finalize(uint64_t traceId, const std::string& conclusion);

    // ── Query ─────────────────────────────────────────────────────────────────

    ReasoningTrace GetTrace(uint64_t traceId) const;
    std::vector<ReasoningTrace> AllTraces()   const;
    std::vector<ReasoningTrace> RecentTraces(uint32_t maxCount) const;

    // ── Rendering / export ────────────────────────────────────────────────────

    /// Produce a plain-text summary suitable for editor overlay or log output.
    std::string FormatTrace(uint64_t traceId) const;

    /// Serialise a trace to a JSON-compatible string.
    std::string SerializeTrace(uint64_t traceId) const;

    /// Callback invoked whenever a trace is finalised.
    void OnTraceFinalized(std::function<void(const ReasoningTrace&)> cb);

    /// Clear all stored traces.
    void Clear();

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace AI
