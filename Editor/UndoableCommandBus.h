#pragma once
/**
 * @file UndoableCommandBus.h
 * @brief IUndoableCommand interface + UndoableCommandBus undo/redo stack.
 *
 * Provides the Editor-layer command infrastructure that EntityCommands.h
 * (Runtime/ECS) depends on.  Commands pushed here are executed immediately;
 * the bus maintains a history so the editor can Undo/Redo arbitrarily.
 *
 * Usage:
 *   auto& bus = UndoableCommandBus::Instance();
 *   bus.Execute(std::make_unique<MyCmd>(args));
 *   bus.Undo();   // reverses most recent command
 *   bus.Redo();   // re-applies most recently undone command
 */

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <cstddef>

namespace Editor {

// ── Forward declaration for ICommand compatibility ─────────────────────────
/// Minimal non-undoable command base (kept for EditorCommandBus compatibility).
struct ICommand {
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual const char* Description() const = 0;
};

/// Undoable command base: adds Undo() to ICommand.
struct IUndoableCommand : ICommand {
    virtual ~IUndoableCommand() = default;
    virtual void Undo() = 0;
};

using CommandPtr         = std::unique_ptr<ICommand>;
using UndoableCommandPtr = std::unique_ptr<IUndoableCommand>;

/// Change callback fired after every execute, undo, or redo.
using HistoryChangedCallback = std::function<void()>;

/// UndoableCommandBus — execute/undo/redo stack for editor operations.
///
/// Thread safety: methods are NOT thread-safe; call from the editor main thread.
class UndoableCommandBus {
public:
    /// Singleton accessor (one bus per editor session).
    static UndoableCommandBus& Instance();

    // ── execution ─────────────────────────────────────────────
    /// Execute cmd immediately and push it onto the undo stack.
    /// Clears the redo stack (standard linear undo behaviour).
    void Execute(UndoableCommandPtr cmd);

    /// Execute a non-undoable command (doesn't touch the undo stack).
    void ExecuteOneShot(CommandPtr cmd);

    // ── undo / redo ───────────────────────────────────────────
    bool CanUndo() const;
    bool CanRedo() const;
    void Undo();
    void Redo();

    // ── inspection ────────────────────────────────────────────
    size_t UndoDepth()  const;
    size_t RedoDepth()  const;
    /// Description of the command that would be undone next.
    std::string NextUndoDescription() const;
    /// Description of the command that would be redone next.
    std::string NextRedoDescription() const;
    /// Full undo history (oldest first).
    std::vector<std::string> UndoHistory() const;

    // ── management ────────────────────────────────────────────
    /// Cap the undo stack at maxDepth entries (default 200).
    void SetMaxDepth(size_t maxDepth);
    /// Discard all undo/redo history.
    void Clear();

    // ── callbacks ─────────────────────────────────────────────
    void OnHistoryChanged(HistoryChangedCallback cb);

private:
    UndoableCommandBus() = default;
    ~UndoableCommandBus() = default;
    UndoableCommandBus(const UndoableCommandBus&) = delete;
    UndoableCommandBus& operator=(const UndoableCommandBus&) = delete;

    std::vector<UndoableCommandPtr>   m_undoStack;
    std::vector<UndoableCommandPtr>   m_redoStack;
    size_t                             m_maxDepth{200};
    std::vector<HistoryChangedCallback> m_callbacks;

    void FireChanged();
};

} // namespace Editor
