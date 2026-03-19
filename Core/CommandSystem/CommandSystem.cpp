#include "Core/CommandSystem/CommandSystem.h"

namespace Core::CommandSystem {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

CommandSystem::CommandSystem(std::size_t maxHistory)
    : m_MaxHistory(maxHistory)
{
}

// ---------------------------------------------------------------------------
// ExecuteCommand
// ---------------------------------------------------------------------------

void CommandSystem::ExecuteCommand(CommandPtr command)
{
    if (!command) return;

    command->Execute();

    // Discard redo branch.
    m_RedoStack.clear();

    m_UndoStack.push_back(std::move(command));

    // Enforce history limit.
    if (m_MaxHistory > 0 && m_UndoStack.size() > m_MaxHistory) {
        m_UndoStack.erase(m_UndoStack.begin());
    }
}

// ---------------------------------------------------------------------------
// Undo
// ---------------------------------------------------------------------------

void CommandSystem::Undo()
{
    if (m_UndoStack.empty()) return;

    auto cmd = std::move(m_UndoStack.back());
    m_UndoStack.pop_back();

    cmd->Undo();

    m_RedoStack.push_back(std::move(cmd));
}

// ---------------------------------------------------------------------------
// Redo
// ---------------------------------------------------------------------------

void CommandSystem::Redo()
{
    if (m_RedoStack.empty()) return;

    auto cmd = std::move(m_RedoStack.back());
    m_RedoStack.pop_back();

    cmd->Execute();

    m_UndoStack.push_back(std::move(cmd));
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

void CommandSystem::Clear()
{
    m_UndoStack.clear();
    m_RedoStack.clear();
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool CommandSystem::CanUndo() const noexcept
{
    return !m_UndoStack.empty();
}

bool CommandSystem::CanRedo() const noexcept
{
    return !m_RedoStack.empty();
}

std::size_t CommandSystem::GetUndoCount() const noexcept
{
    return m_UndoStack.size();
}

std::size_t CommandSystem::GetRedoCount() const noexcept
{
    return m_RedoStack.size();
}

std::string CommandSystem::GetUndoName() const
{
    if (m_UndoStack.empty()) return {};
    return m_UndoStack.back()->GetName();
}

std::string CommandSystem::GetRedoName() const
{
    if (m_RedoStack.empty()) return {};
    return m_RedoStack.back()->GetName();
}

} // namespace Core::CommandSystem
