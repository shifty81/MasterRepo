#include "Runtime/Components/ComponentRegistry/ComponentRegistry.h"

namespace Runtime {

ComponentRegistry::ComponentRegistry() = default;
ComponentRegistry::~ComponentRegistry() = default;

const ComponentInfo* ComponentRegistry::Info(ComponentTypeId id) const {
    for (const auto& info : m_infos)
        if (info.id == id) return &info;
    return nullptr;
}

const ComponentInfo* ComponentRegistry::InfoByName(const std::string& name) const {
    for (const auto& info : m_infos)
        if (info.name == name) return &info;
    return nullptr;
}

bool ComponentRegistry::IsRegistered(ComponentTypeId id) const {
    return Info(id) != nullptr;
}

size_t ComponentRegistry::Count() const {
    return m_infos.size();
}

const std::vector<ComponentInfo>& ComponentRegistry::All() const {
    return m_infos;
}

void* ComponentRegistry::Create(ComponentTypeId id) const {
    const ComponentInfo* info = Info(id);
    if (!info || !info->factory) return nullptr;
    return info->factory();
}

void ComponentRegistry::Clear() {
    m_infos.clear();
    m_typeMap.clear();
    m_nextId = 0;
}

} // namespace Runtime
