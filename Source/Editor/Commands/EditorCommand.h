#pragma once
#include <memory>
#include <vector>

namespace NF::Editor {

/// @brief Abstract base for all undoable editor operations.
class EditorCommand {
public:
    virtual ~EditorCommand() = default;

    /// @brief Apply the command.
    virtual void Execute() = 0;

    /// @brief Reverse the command.
    virtual void Undo() = 0;
};

/// @brief Manages an undo/redo stack of editor commands.
class CommandHistory {
public:
    /// @brief Push and immediately execute a command.
    /// @param cmd Command to record; ownership is transferred.
    void Push(std::shared_ptr<EditorCommand> cmd);

    /// @brief Undo the most recent command.
    void Undo();

    /// @brief Redo the most recently undone command.
    void Redo();

    /// @brief Returns true if there is a command available to undo.
    [[nodiscard]] bool CanUndo() const noexcept { return !m_Undo.empty(); }

    /// @brief Returns true if there is a command available to redo.
    [[nodiscard]] bool CanRedo() const noexcept { return !m_Redo.empty(); }

private:
    std::vector<std::shared_ptr<EditorCommand>> m_Undo;
    std::vector<std::shared_ptr<EditorCommand>> m_Redo;
};

} // namespace NF::Editor
