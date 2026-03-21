#include "Editor/UndoableCommandBus.h"

namespace Editor {

UndoableCommandBus& UndoableCommandBus::Instance() {
    static UndoableCommandBus s_instance;
    return s_instance;
}

void UndoableCommandBus::Execute(UndoableCommandPtr cmd) {
    if (!cmd) return;
    cmd->Execute();
    m_redoStack.clear();
    m_undoStack.push_back(std::move(cmd));
    // Trim oldest if over cap.
    while (m_undoStack.size() > m_maxDepth)
        m_undoStack.erase(m_undoStack.begin());
    FireChanged();
}

void UndoableCommandBus::ExecuteOneShot(CommandPtr cmd) {
    if (!cmd) return;
    cmd->Execute();
}

bool UndoableCommandBus::CanUndo() const { return !m_undoStack.empty(); }
bool UndoableCommandBus::CanRedo() const { return !m_redoStack.empty(); }

void UndoableCommandBus::Undo() {
    if (m_undoStack.empty()) return;
    auto& cmd = m_undoStack.back();
    cmd->Undo();
    m_redoStack.push_back(std::move(cmd));
    m_undoStack.pop_back();
    FireChanged();
}

void UndoableCommandBus::Redo() {
    if (m_redoStack.empty()) return;
    auto& cmd = m_redoStack.back();
    cmd->Execute();
    m_undoStack.push_back(std::move(cmd));
    m_redoStack.pop_back();
    FireChanged();
}

size_t UndoableCommandBus::UndoDepth() const { return m_undoStack.size(); }
size_t UndoableCommandBus::RedoDepth() const { return m_redoStack.size(); }

std::string UndoableCommandBus::NextUndoDescription() const {
    if (m_undoStack.empty()) return {};
    return m_undoStack.back()->Description();
}

std::string UndoableCommandBus::NextRedoDescription() const {
    if (m_redoStack.empty()) return {};
    return m_redoStack.back()->Description();
}

std::vector<std::string> UndoableCommandBus::UndoHistory() const {
    std::vector<std::string> hist;
    hist.reserve(m_undoStack.size());
    for (const auto& cmd : m_undoStack)
        hist.push_back(cmd->Description());
    return hist;
}

void UndoableCommandBus::SetMaxDepth(size_t maxDepth) { m_maxDepth = maxDepth; }

void UndoableCommandBus::Clear() {
    m_undoStack.clear();
    m_redoStack.clear();
    FireChanged();
}

void UndoableCommandBus::OnHistoryChanged(HistoryChangedCallback cb) {
    m_callbacks.push_back(std::move(cb));
}

void UndoableCommandBus::FireChanged() {
    for (auto& cb : m_callbacks) cb();
}

} // namespace Editor
