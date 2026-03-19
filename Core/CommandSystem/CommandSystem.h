#pragma once

#include <cstddef>
#include <vector>

#include "Core/CommandSystem/Command.h"

namespace Core::CommandSystem {

/// Undo / redo command manager.
///
/// Maintains a linear history of executed commands with a configurable
/// maximum depth.  When a new command is executed after one or more undos,
/// the redo branch is discarded (standard linear undo model).
///
/// Usage:
///   Core::CommandSystem::CommandSystem cmds(100);  // keep 100 levels
///   cmds.ExecuteCommand(std::make_unique<MoveCommand>(entity, newPos));
///   cmds.Undo();
///   cmds.Redo();
class CommandSystem {
public:
    /// @param maxHistory  Maximum number of undo levels to retain (0 = unlimited).
    explicit CommandSystem(std::size_t maxHistory = 100);
    ~CommandSystem() = default;

    CommandSystem(const CommandSystem&)            = delete;
    CommandSystem& operator=(const CommandSystem&) = delete;

    /// Execute a command and push it onto the undo stack.
    /// Any pending redo history is discarded.
    void ExecuteCommand(CommandPtr command);

    /// Undo the most recent command.  No-op if nothing to undo.
    void Undo();

    /// Redo the last undone command.  No-op if nothing to redo.
    void Redo();

    /// Clear all history (undo and redo).
    void Clear();

    // --- Queries ---

    [[nodiscard]] bool        CanUndo() const noexcept;
    [[nodiscard]] bool        CanRedo() const noexcept;
    [[nodiscard]] std::size_t GetUndoCount() const noexcept;
    [[nodiscard]] std::size_t GetRedoCount() const noexcept;

    /// Return the name of the command that would be undone, or empty string.
    [[nodiscard]] std::string GetUndoName() const;
    /// Return the name of the command that would be redone, or empty string.
    [[nodiscard]] std::string GetRedoName() const;

private:
    std::size_t              m_MaxHistory;
    std::vector<CommandPtr>  m_UndoStack;
    std::vector<CommandPtr>  m_RedoStack;
};

} // namespace Core::CommandSystem
