#include "Editor/Panels/Inspector/InspectorTools.h"
#include <algorithm>

namespace Editor {

// EntityInspectorTool

void EntityInspectorTool::Inspect(uint32_t entityId) {
    m_entityId = entityId;
    m_components.clear();
}

void EntityInspectorTool::AddComponent(
        const std::string& name,
        const std::vector<std::pair<std::string,std::string>>& fields) {
    m_components.push_back({name, fields});
}

void EntityInspectorTool::ClearInspection() {
    m_entityId = 0;
    m_components.clear();
}

// MaterialOverrideTool

void MaterialOverrideTool::SetParam(const std::string& name, float value) {
    for (auto& p : m_params)
        if (p.name == name) { p.value = value; return; }
    m_params.push_back({name, value});
}

float MaterialOverrideTool::GetParam(const std::string& name) const {
    for (const auto& p : m_params)
        if (p.name == name) return p.value;
    return 0.0f;
}

bool MaterialOverrideTool::HasParam(const std::string& name) const {
    return std::any_of(m_params.begin(), m_params.end(),
        [&name](const MaterialParam& p){ return p.name == name; });
}

void MaterialOverrideTool::ResetOverrides() { m_params.clear(); }

} // namespace Editor
