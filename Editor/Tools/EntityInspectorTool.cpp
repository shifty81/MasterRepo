#include "Editor/Tools/EntityInspectorTool.h"

namespace Editor {

void EntityInspectorTool::Inspect(uint32_t entityId) {
    m_entityId = entityId;
    m_components.clear();
}

void EntityInspectorTool::AddComponent(
    const std::string& name,
    const std::vector<std::pair<std::string, std::string>>& fields)
{
    InspectedComponent comp;
    comp.name   = name;
    comp.fields = fields;
    m_components.push_back(std::move(comp));
}

void EntityInspectorTool::ClearInspection() {
    m_entityId = 0;
    m_components.clear();
}

} // namespace Editor
