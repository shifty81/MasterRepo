#pragma once
#include <string>
#include <vector>
#include <functional>

namespace AI {

/// A single step in the AI's reasoning chain.
struct ReasoningStep {
    std::string system;      ///< Subsystem that produced this step (e.g. "PCG", "Shader")
    std::string description; ///< Human-readable explanation
    float       confidence;  ///< 0.0 – 1.0
};

/// Full reasoning trace for one AI decision.
struct DecisionTrace {
    std::string              decisionId;  ///< Unique identifier
    std::string              actionTaken; ///< What the AI decided to do
    std::vector<ReasoningStep> steps;     ///< Ordered reasoning chain
    float                    overallConf; ///< Aggregate confidence
};

using TraceCallback = std::function<void(const DecisionTrace&)>;

/// DecisionVisualizer — records and exposes AI reasoning chains.
///
/// Subsystems push reasoning steps here; the editor overlays query the
/// visualizer to display "why the AI made this choice" annotations.
class DecisionVisualizer {
public:
    DecisionVisualizer();
    ~DecisionVisualizer();

    // ── recording ─────────────────────────────────────────────
    /// Begin a new decision trace.  Returns the decisionId.
    std::string BeginTrace(const std::string& actionTaken);
    /// Append a reasoning step to the active trace.
    void        AddStep(const std::string& decisionId,
                        const ReasoningStep& step);
    /// Finalise and store the trace.  Fires OnTrace callbacks.
    void        CommitTrace(const std::string& decisionId);
    /// Discard a trace without committing.
    void        DiscardTrace(const std::string& decisionId);

    // ── query ─────────────────────────────────────────────────
    /// Most-recent N completed traces (newest first).
    std::vector<DecisionTrace> GetRecentTraces(size_t maxCount = 20) const;
    /// Retrieve a single trace by id (returns empty trace if not found).
    DecisionTrace              GetTrace(const std::string& decisionId) const;
    /// Clear all stored traces.
    void                       ClearHistory();

    // ── callbacks ─────────────────────────────────────────────
    void OnTrace(TraceCallback cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace AI
