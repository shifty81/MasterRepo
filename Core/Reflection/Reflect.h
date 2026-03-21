#pragma once

#include "Core/Reflection/TypeInfo.h"

#include <cstddef>
#include <cstring>
#include <type_traits>

namespace Core {

// --- Helper: iterate all properties of a reflected type ---

inline void ForEachProperty(TypeID typeID,
                            const std::function<void(const Property&)>& visitor) {
    const TypeInfo* info = TypeRegistry::Instance().Find(typeID);
    if (!info) return;
    for (const auto& prop : info->Properties) {
        visitor(prop);
    }
}

inline void ForEachProperty(std::string_view typeName,
                            const std::function<void(const Property&)>& visitor) {
    ForEachProperty(HashType(typeName), visitor);
}

} // namespace Core

// --- Reflection registration macros ---
//
// Usage:
//
//   struct Vec3 { float x, y, z; };
//
//   REFLECT_BEGIN(Vec3)
//       REFLECT_PROPERTY(x, float)
//       REFLECT_PROPERTY(y, float)
//       REFLECT_PROPERTY(z, float)
//   REFLECT_END()
//
// This registers Vec3 with the TypeRegistry at static-init time.

#define REFLECT_BEGIN(ClassName)                                                 \
    namespace {                                                                 \
    inline const bool ClassName##_registered = [] {                             \
            using T = ClassName;                                                \
            Core::TypeInfo info;                                                \
            info.Name = #ClassName;                                             \
            info.Size = sizeof(T);                                              \
            info.ID   = Core::HashType(#ClassName);                            \
            info.BaseID = 0;

#define REFLECT_BEGIN_DERIVED(ClassName, BaseClassName)                         \
    namespace {                                                                 \
    inline const bool ClassName##_registered = [] {                             \
            using T = ClassName;                                                \
            Core::TypeInfo info;                                                \
            info.Name = #ClassName;                                             \
            info.Size = sizeof(T);                                              \
            info.ID   = Core::HashType(#ClassName);                            \
            info.BaseID = Core::HashType(#BaseClassName);

#define REFLECT_PROPERTY(propName, propType)                                     \
            {                                                                   \
                Core::Property prop;                                            \
                prop.Name     = #propName;                                      \
                prop.TypeName = #propType;                                      \
                prop.Offset   = offsetof(T, propName);                          \
                prop.Size     = sizeof(propType);                               \
                prop.Getter = [](const void* instance, void* outValue) {        \
                    const auto* obj = static_cast<const T*>(instance);          \
                    std::memcpy(outValue, &(obj->propName), sizeof(propType));   \
                };                                                              \
                prop.Setter = [](void* instance, const void* inValue) {         \
                    auto* obj = static_cast<T*>(instance);                      \
                    std::memcpy(&(obj->propName), inValue, sizeof(propType));    \
                };                                                              \
                info.Properties.push_back(std::move(prop));                     \
            }

#define REFLECT_END()                                                           \
            Core::TypeRegistry::Instance().Register(std::move(info));           \
            return true;                                                        \
        }();                                                                    \
    } // anonymous namespace
