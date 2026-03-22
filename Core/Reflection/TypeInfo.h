#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace Core {

// --- Compile-time string hashing for type IDs ---
// Same FNV-1a 64-bit hash used in Core/Events/Event.h.

using TypeID = uint64_t;

constexpr TypeID HashType(std::string_view name) noexcept {
    uint64_t hash = 14695981039346656037ULL;
    for (char c : name) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

// --- Property descriptor ---

struct Property {
    std::string Name;
    std::string TypeName;
    std::size_t Offset = 0;
    std::size_t Size   = 0;

    // Type-erased getter/setter operating on a void* instance pointer.
    // Getter copies the property value into the provided output buffer.
    // Setter copies from the provided input buffer into the instance.
    std::function<void(const void* instance, void* outValue)> Getter;
    std::function<void(void* instance, const void* inValue)>  Setter;
};

// --- TypeInfo descriptor ---

class TypeRegistry; // forward declaration — needed by TypeInfo::FindProperty

struct TypeInfo {
    std::string            Name;
    std::size_t            Size   = 0;
    TypeID                 ID     = 0;
    std::vector<Property>  Properties;
    TypeID                 BaseID = 0; // 0 = no base

    // Implemented out-of-line in TypeInfo.cpp where TypeRegistry is complete.
    const Property* FindProperty(std::string_view propName) const;
};

// --- TypeRegistry singleton ---

class TypeRegistry {
public:
    static TypeRegistry& Instance();

    void               Register(TypeInfo info);
    const TypeInfo*     Find(TypeID id) const;
    const TypeInfo*     Find(std::string_view name) const;

    // Iterate all registered types.
    const std::unordered_map<TypeID, TypeInfo>& AllTypes() const { return m_Types; }

private:
    TypeRegistry() = default;

    std::unordered_map<TypeID, TypeInfo> m_Types;
};

} // namespace Core
