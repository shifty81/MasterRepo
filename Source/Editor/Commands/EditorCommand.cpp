#include "Editor/Commands/EditorCommand.h"

namespace NF::Editor {

void CommandHistory::Push(std::shared_ptr<EditorCommand> cmd) {
    cmd->Execute();
    m_Undo.emplace_back(std::move(cmd));
    m_Redo.clear(); // New action invalidates the redo stack.
}

void CommandHistory::Undo() {
    if (m_Undo.empty()) return;
    auto cmd = m_Undo.back();
    m_Undo.pop_back();
    cmd->Undo();
    m_Redo.emplace_back(std::move(cmd));
}

void CommandHistory::Redo() {
    if (m_Redo.empty()) return;
    auto cmd = m_Redo.back();
    m_Redo.pop_back();
    cmd->Execute();
    m_Undo.emplace_back(std::move(cmd));
}

} // namespace NF::Editor
