#pragma once
#include <cstddef>
#include <string_view>
#include <typeindex>

namespace NF {

/// @brief Lightweight compile-time/runtime type descriptor.
struct TypeInfo {
    /// @brief Implementation-defined type name (from typeid().name()).
    std::string_view Name;

    /// @brief sizeof(T) for the described type.
    size_t Size{0};

    /// @brief Stable hash-code identifier derived from std::type_index.
    size_t TypeId{0};
};

/// @brief Retrieve the singleton TypeInfo descriptor for type T.
/// @tparam T The type to describe.
/// @return A const reference to the static TypeInfo instance for T.
template<typename T>
inline const TypeInfo& TypeOf() {
    static const TypeInfo info{
        std::string_view(typeid(T).name()),
        sizeof(T),
        std::type_index(typeid(T)).hash_code()
    };
    return info;
}

} // namespace NF
