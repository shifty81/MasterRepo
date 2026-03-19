#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace Core::Serialization {

/// Lightweight JSON value representation.
///
/// Wraps the six JSON types (null, bool, number-int, number-double, string,
/// array, object) without depending on any third-party library.  Designed as
/// the interchange format for the engine's serialization pipeline.
class JsonValue {
public:
    /// JSON type discriminator.
    enum class Type { Null, Bool, Int, Double, String, Array, Object };

    using ArrayType  = std::vector<JsonValue>;
    using ObjectType = std::map<std::string, JsonValue>;

    // --- Constructors ---

    JsonValue()                        : m_Data(nullptr)           {}
    JsonValue(std::nullptr_t)          : m_Data(nullptr)           {}
    JsonValue(bool v)                  : m_Data(v)                 {}
    JsonValue(int v)                   : m_Data(static_cast<int64_t>(v)) {}
    JsonValue(int64_t v)               : m_Data(v)                 {}
    JsonValue(double v)                : m_Data(v)                 {}
    JsonValue(const char* v)           : m_Data(std::string(v))    {}
    JsonValue(std::string v)           : m_Data(std::move(v))      {}
    JsonValue(ArrayType v)             : m_Data(std::move(v))      {}
    JsonValue(ObjectType v)            : m_Data(std::move(v))      {}

    // --- Type queries ---

    [[nodiscard]] Type GetType() const noexcept;
    [[nodiscard]] bool IsNull()   const noexcept { return GetType() == Type::Null;   }
    [[nodiscard]] bool IsBool()   const noexcept { return GetType() == Type::Bool;   }
    [[nodiscard]] bool IsInt()    const noexcept { return GetType() == Type::Int;    }
    [[nodiscard]] bool IsDouble() const noexcept { return GetType() == Type::Double; }
    [[nodiscard]] bool IsString() const noexcept { return GetType() == Type::String; }
    [[nodiscard]] bool IsArray()  const noexcept { return GetType() == Type::Array;  }
    [[nodiscard]] bool IsObject() const noexcept { return GetType() == Type::Object; }
    [[nodiscard]] bool IsNumber() const noexcept { return IsInt() || IsDouble(); }

    // --- Accessors (throw std::bad_variant_access on type mismatch) ---

    [[nodiscard]] bool               AsBool()   const { return std::get<bool>(m_Data);        }
    [[nodiscard]] int64_t            AsInt()    const { return std::get<int64_t>(m_Data);     }
    [[nodiscard]] double             AsDouble() const;
    [[nodiscard]] const std::string& AsString() const { return std::get<std::string>(m_Data); }
    [[nodiscard]] const ArrayType&   AsArray()  const { return std::get<ArrayType>(m_Data);   }
    [[nodiscard]] const ObjectType&  AsObject() const { return std::get<ObjectType>(m_Data);  }

    [[nodiscard]] ArrayType&         AsArray()        { return std::get<ArrayType>(m_Data);   }
    [[nodiscard]] ObjectType&        AsObject()       { return std::get<ObjectType>(m_Data);  }

    /// Object member access by key.  Returns a Null JsonValue if not found.
    [[nodiscard]] const JsonValue& operator[](const std::string& key) const;

    /// Array element access by index.  UB if out of range.
    [[nodiscard]] const JsonValue& operator[](std::size_t index) const;

private:
    using DataVariant = std::variant<
        std::nullptr_t,
        bool,
        int64_t,
        double,
        std::string,
        ArrayType,
        ObjectType
    >;
    DataVariant m_Data;
};

// ---------------------------------------------------------------------------
// JsonSerializer — serialize / deserialize JsonValue ↔ JSON string
// ---------------------------------------------------------------------------

/// Minimal, dependency-free JSON serializer and parser.
///
/// Usage:
///   Core::Serialization::JsonValue obj(JsonValue::ObjectType{
///       {"name", "player"},
///       {"hp",   100},
///   });
///   std::string json = JsonSerializer::SerializeToString(obj);
///   auto parsed = JsonSerializer::DeserializeFromString(json);
class JsonSerializer {
public:
    /// Serialize a JsonValue tree into a compact JSON string.
    [[nodiscard]] static std::string SerializeToString(const JsonValue& value);

    /// Serialize with indentation for human readability.
    [[nodiscard]] static std::string SerializeToPrettyString(const JsonValue& value, int indentWidth = 2);

    /// Parse a JSON string into a JsonValue tree.
    /// Returns a Null JsonValue on parse failure.
    [[nodiscard]] static JsonValue DeserializeFromString(std::string_view json);
};

} // namespace Core::Serialization
