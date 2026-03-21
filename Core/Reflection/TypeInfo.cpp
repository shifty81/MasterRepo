#include "Core/Reflection/TypeInfo.h"

namespace Core {

TypeRegistry& TypeRegistry::Instance() {
    static TypeRegistry instance;
    return instance;
}

void TypeRegistry::Register(TypeInfo info) {
    TypeID id = info.ID;
    m_Types.insert_or_assign(id, std::move(info));
}

const TypeInfo* TypeRegistry::Find(TypeID id) const {
    auto it = m_Types.find(id);
    return (it != m_Types.end()) ? &it->second : nullptr;
}

const TypeInfo* TypeRegistry::Find(std::string_view name) const {
    TypeID id = HashType(name);
    return Find(id);
}

const Property* TypeInfo::FindProperty(std::string_view propName) const {
    for (const auto& p : Properties) {
        if (p.Name == propName) return &p;
    }
    if (BaseID != 0) {
        const TypeInfo* base = TypeRegistry::Instance().Find(BaseID);
        if (base) return base->FindProperty(propName);
    }
    return nullptr;
}

} // namespace Core
