#include "Editor/Tools/EditorToolRegistry.h"

namespace Editor {

void EditorToolRegistry::Register(std::unique_ptr<ITool> tool) {
    m_tools.push_back(std::move(tool));
}

size_t EditorToolRegistry::Count() const { return m_tools.size(); }

ITool* EditorToolRegistry::Get(size_t index) const {
    return index < m_tools.size() ? m_tools[index].get() : nullptr;
}

ITool* EditorToolRegistry::FindByName(const std::string& name) const {
    for (auto& t : m_tools) if (t->Name() == name) return t.get();
    return nullptr;
}

bool EditorToolRegistry::Activate(const std::string& name) {
    ITool* tool = FindByName(name);
    if (!tool) return false;
    if (m_active && m_active != tool) m_active->Deactivate();
    m_active = tool;
    m_active->Activate();
    return true;
}

void EditorToolRegistry::DeactivateCurrent() {
    if (m_active) { m_active->Deactivate(); m_active = nullptr; }
}

ITool* EditorToolRegistry::ActiveTool() const { return m_active; }

void EditorToolRegistry::UpdateActive(float dt) {
    if (m_active) m_active->Update(dt);
}

std::vector<std::string> EditorToolRegistry::ToolNames() const {
    std::vector<std::string> out;
    for (auto& t : m_tools) out.push_back(t->Name());
    return out;
}

void EditorToolRegistry::Clear() {
    DeactivateCurrent();
    m_tools.clear();
}

} // namespace Editor
