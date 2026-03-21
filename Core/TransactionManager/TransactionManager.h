#pragma once
#include <string>
#include <vector>
#include <functional>
#include <any>

namespace Core {

/// A single staged change.
struct StagedChange {
    std::string targetId;    ///< Object / file / entity being modified
    std::string description; ///< Human-readable summary
    std::any    oldValue;    ///< Previous state (for rollback)
    std::any    newValue;    ///< Proposed new state
};

/// Status of a transaction.
enum class TransactionStatus { Idle, Active, Committed, RolledBack };

using CommitCallback   = std::function<void(const std::vector<StagedChange>&)>;
using RollbackCallback = std::function<void(const std::vector<StagedChange>&)>;

/// TransactionManager — stage AI suggestions before applying them.
///
/// AI pipelines stage changes here; nothing touches the live project until
/// Commit() is called.  If validation fails, Rollback() restores the
/// previous state.  Multiple nested transactions are supported via a stack.
class TransactionManager {
public:
    TransactionManager();
    ~TransactionManager();

    // ── transaction lifecycle ─────────────────────────────────
    /// Open a new transaction.  Nested Begin() calls create a save-point.
    void Begin(const std::string& label = "");
    /// Apply all staged changes to the live project.
    bool Commit();
    /// Discard all staged changes and restore previous state.
    void Rollback();

    // ── staging ───────────────────────────────────────────────
    /// Stage a change in the active transaction.
    void Stage(const StagedChange& change);
    /// Stage multiple changes at once.
    void StageAll(const std::vector<StagedChange>& changes);

    // ── query ─────────────────────────────────────────────────
    TransactionStatus            Status()        const;
    size_t                       StagedCount()   const;
    std::vector<StagedChange>    GetStaged()     const;
    std::string                  CurrentLabel()  const;
    int                          Depth()         const; ///< Nesting level

    // ── callbacks ─────────────────────────────────────────────
    void OnCommit(CommitCallback cb);
    void OnRollback(RollbackCallback cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
