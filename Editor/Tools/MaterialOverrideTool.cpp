#include "Editor/Tools/MaterialOverrideTool.h"
#include <algorithm>

namespace Editor {

void MaterialOverrideTool::SetTarget(uint32_t entityId) {
    m_target = entityId;
}

void MaterialOverrideTool::SetParam(const std::string& name, float value) {
    for (auto& p : m_params) {
        if (p.name == name) { p.value = value; return; }
    }
    m_params.push_back({name, value});
}

float MaterialOverrideTool::GetParam(const std::string& name) const {
    for (auto& p : m_params) if (p.name == name) return p.value;
    return 0.0f;
}

bool MaterialOverrideTool::HasParam(const std::string& name) const {
    for (auto& p : m_params) if (p.name == name) return true;
    return false;
}

void MaterialOverrideTool::ResetOverrides() { m_params.clear(); }

} // namespace Editor
