#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace AI {

/// A suggestion produced by one AI subsystem pipeline.
struct AISuggestion {
    std::string id;          ///< Unique suggestion identifier
    std::string pipeline;    ///< Source pipeline: "PCG", "Shader", "Gameplay", etc.
    std::string description; ///< Human-readable summary
    int         priority;    ///< Higher = more important (default 0)
    float       confidence;  ///< 0.0 – 1.0
};

/// Result of a conflict resolution attempt.
struct ResolutionResult {
    std::string             winnerId;    ///< Accepted suggestion id (empty if discarded)
    std::vector<std::string> discardedIds;
    std::string             reason;
    bool                    hadConflict{false};
};

using ConflictCallback = std::function<void(const ResolutionResult&)>;

/// ConflictResolver — arbitrates between competing AI suggestions.
///
/// When multiple AI pipelines (PCG, Shader, Gameplay, etc.) produce
/// conflicting suggestions for the same target, the resolver selects a
/// winner based on priority, confidence, and user-defined pipeline weights.
/// Cyclic dependency chains are detected and broken automatically.
class ConflictResolver {
public:
    ConflictResolver();
    ~ConflictResolver();

    // ── configuration ─────────────────────────────────────────
    /// Set the relative weight for a named pipeline (default 1.0).
    void SetPipelineWeight(const std::string& pipeline, float weight);
    /// Register a dependency: pipelineA must resolve before pipelineB.
    /// Cycles are detected at resolution time and broken by priority.
    void AddDependency(const std::string& from, const std::string& to);
    void ClearDependencies();

    // ── suggestion queue ──────────────────────────────────────
    /// Submit a suggestion for consideration.
    void Submit(const AISuggestion& suggestion);
    /// Clear all pending suggestions.
    void ClearPending();
    std::vector<AISuggestion> GetPending() const;

    // ── resolution ────────────────────────────────────────────
    /// Resolve all pending suggestions, returning ordered accepted list.
    /// Conflicting suggestions are arbitrated; conflicts fire the callback.
    std::vector<AISuggestion> Resolve();

    // ── callbacks ─────────────────────────────────────────────
    void OnConflict(ConflictCallback cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace AI
