#pragma once
/**
 * @file SuggestionSandbox.h
 * @brief Multi-step AI suggestion sandbox with undo/redo support.
 *
 * AI suggestions are staged in a sandbox before being committed to the live
 * project.  Each staged suggestion can be previewed, accepted, rejected, or
 * rolled back independently.  An undo stack allows reversing previously
 * committed suggestions.
 *
 * Typical usage:
 * @code
 *   SuggestionSandbox sb;
 *   sb.Init();
 *   uint64_t sid = sb.Stage("refactor_class", "Rename Foo → Bar in 3 files");
 *   sb.SetPayload(sid, diffJson);
 *   sb.Preview(sid);
 *   sb.Commit(sid);   // applies change to project
 *   sb.Undo();        // reverts last committed suggestion
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace AI {

// ── Lifecycle states for a staged suggestion ──────────────────────────────────

enum class SuggestionState : uint8_t {
    Staged    = 0,
    Previewed = 1,
    Committed = 2,
    Rejected  = 3,
    RolledBack = 4,
};

// ── One AI suggestion in the sandbox ─────────────────────────────────────────

struct StagedSuggestion {
    uint64_t        id{0};
    std::string     tag;           ///< action label, e.g. "refactor_class"
    std::string     description;
    std::string     payload;       ///< JSON diff, code patch, or structured data
    SuggestionState state{SuggestionState::Staged};
    float           confidence{0.0f};
    uint64_t        createdMs{0};
    uint64_t        committedMs{0};
};

// ── SuggestionSandbox ─────────────────────────────────────────────────────────

class SuggestionSandbox {
public:
    SuggestionSandbox();
    ~SuggestionSandbox();

    void Init();
    void Shutdown();

    // ── Staging ───────────────────────────────────────────────────────────────

    uint64_t Stage(const std::string& tag, const std::string& description,
                   float confidence = 0.0f);
    void     SetPayload(uint64_t id, const std::string& payload);
    void     Preview(uint64_t id);
    void     Commit(uint64_t id);
    void     Reject(uint64_t id);

    // ── Undo / redo ───────────────────────────────────────────────────────────

    bool     CanUndo()  const;
    bool     CanRedo()  const;
    void     Undo();
    void     Redo();
    uint32_t UndoDepth() const;

    // ── Query ─────────────────────────────────────────────────────────────────

    StagedSuggestion               GetSuggestion(uint64_t id) const;
    std::vector<StagedSuggestion>  AllSuggestions()           const;
    std::vector<StagedSuggestion>  StagedSuggestions()        const;
    std::vector<StagedSuggestion>  CommittedSuggestions()     const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnCommit(std::function<void(const StagedSuggestion&)> cb);
    void OnRollback(std::function<void(const StagedSuggestion&)> cb);
    void OnReject(std::function<void(const StagedSuggestion&)> cb);

    /// Clear the sandbox and undo history.
    void Clear();

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace AI
