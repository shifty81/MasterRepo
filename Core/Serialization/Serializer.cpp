#include "Core/Serialization/Serializer.h"

#include <cassert>
#include <cctype>
#include <charconv>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace Core::Serialization {

// ===========================================================================
// JsonValue
// ===========================================================================

JsonValue::Type JsonValue::GetType() const noexcept
{
    return static_cast<Type>(m_Data.index());
}

double JsonValue::AsDouble() const
{
    if (IsInt()) return static_cast<double>(std::get<int64_t>(m_Data));
    return std::get<double>(m_Data);
}

static const JsonValue s_NullValue{};

const JsonValue& JsonValue::operator[](const std::string& key) const
{
    if (!IsObject()) return s_NullValue;
    auto& obj = std::get<JsonValue::ObjectType>(m_Data);
    auto it = obj.find(key);
    if (it == obj.end()) return s_NullValue;
    return it->second;
}

const JsonValue& JsonValue::operator[](std::size_t index) const
{
    return std::get<ArrayType>(m_Data).at(index);
}

// ===========================================================================
// Serialisation helpers (private)
// ===========================================================================

namespace {

void EscapeString(const std::string& s, std::string& out)
{
    out += '"';
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    out += '"';
}

void SerializeImpl(const JsonValue& value, std::string& out)
{
    switch (value.GetType()) {
        case JsonValue::Type::Null:   out += "null"; break;
        case JsonValue::Type::Bool:   out += value.AsBool() ? "true" : "false"; break;
        case JsonValue::Type::Int:    out += std::to_string(value.AsInt()); break;
        case JsonValue::Type::Double: {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.17g", value.AsDouble());
            out += buf;
            break;
        }
        case JsonValue::Type::String:
            EscapeString(value.AsString(), out);
            break;
        case JsonValue::Type::Array: {
            out += '[';
            const auto& arr = value.AsArray();
            for (std::size_t i = 0; i < arr.size(); ++i) {
                if (i > 0) out += ',';
                SerializeImpl(arr[i], out);
            }
            out += ']';
            break;
        }
        case JsonValue::Type::Object: {
            out += '{';
            bool first = true;
            for (const auto& [k, v] : value.AsObject()) {
                if (!first) out += ',';
                first = false;
                EscapeString(k, out);
                out += ':';
                SerializeImpl(v, out);
            }
            out += '}';
            break;
        }
    }
}

void SerializePrettyImpl(const JsonValue& value, std::string& out, int indent, int currentIndent)
{
    auto writeIndent = [&](int level) {
        out.append(static_cast<std::size_t>(level), ' ');
    };

    switch (value.GetType()) {
        case JsonValue::Type::Null:
        case JsonValue::Type::Bool:
        case JsonValue::Type::Int:
        case JsonValue::Type::Double:
        case JsonValue::Type::String:
            SerializeImpl(value, out);
            break;
        case JsonValue::Type::Array: {
            const auto& arr = value.AsArray();
            if (arr.empty()) { out += "[]"; break; }
            out += "[\n";
            for (std::size_t i = 0; i < arr.size(); ++i) {
                writeIndent(currentIndent + indent);
                SerializePrettyImpl(arr[i], out, indent, currentIndent + indent);
                if (i + 1 < arr.size()) out += ',';
                out += '\n';
            }
            writeIndent(currentIndent);
            out += ']';
            break;
        }
        case JsonValue::Type::Object: {
            const auto& obj = value.AsObject();
            if (obj.empty()) { out += "{}"; break; }
            out += "{\n";
            std::size_t idx = 0;
            for (const auto& [k, v] : obj) {
                writeIndent(currentIndent + indent);
                EscapeString(k, out);
                out += ": ";
                SerializePrettyImpl(v, out, indent, currentIndent + indent);
                if (idx + 1 < obj.size()) out += ',';
                out += '\n';
                ++idx;
            }
            writeIndent(currentIndent);
            out += '}';
            break;
        }
    }
}

} // anonymous namespace

// ===========================================================================
// JsonSerializer — public API
// ===========================================================================

std::string JsonSerializer::SerializeToString(const JsonValue& value)
{
    std::string out;
    SerializeImpl(value, out);
    return out;
}

std::string JsonSerializer::SerializeToPrettyString(const JsonValue& value, int indentWidth)
{
    std::string out;
    SerializePrettyImpl(value, out, indentWidth, 0);
    return out;
}

// ===========================================================================
// Parser (private)
// ===========================================================================

namespace {

class Parser {
public:
    explicit Parser(std::string_view input) : m_Input(input) {}

    JsonValue Parse()
    {
        SkipWhitespace();
        auto value = ParseValue();
        SkipWhitespace();
        return value;
    }

private:
    std::string_view m_Input;
    std::size_t      m_Pos = 0;

    // --- Utilities ---

    [[nodiscard]] bool AtEnd() const noexcept { return m_Pos >= m_Input.size(); }

    [[nodiscard]] char Peek() const
    {
        if (AtEnd()) return '\0';
        return m_Input[m_Pos];
    }

    char Advance()
    {
        if (AtEnd()) return '\0';
        return m_Input[m_Pos++];
    }

    bool Match(char c)
    {
        if (Peek() == c) { ++m_Pos; return true; }
        return false;
    }

    void Expect(char c)
    {
        if (!Match(c)) {
            throw std::runtime_error(std::string("JSON parse error: expected '") + c + "'");
        }
    }

    void SkipWhitespace()
    {
        while (!AtEnd() && std::isspace(static_cast<unsigned char>(Peek()))) {
            ++m_Pos;
        }
    }

    // --- Parse routines ---

    JsonValue ParseValue()
    {
        SkipWhitespace();
        if (AtEnd()) return JsonValue{};

        char c = Peek();
        switch (c) {
            case '"': return ParseString();
            case '{': return ParseObject();
            case '[': return ParseArray();
            case 't': case 'f': return ParseBool();
            case 'n': return ParseNull();
            default:
                if (c == '-' || std::isdigit(static_cast<unsigned char>(c)))
                    return ParseNumber();
                return JsonValue{};
        }
    }

    JsonValue ParseNull()
    {
        if (m_Input.substr(m_Pos, 4) == "null") {
            m_Pos += 4;
            return JsonValue{nullptr};
        }
        return JsonValue{};
    }

    JsonValue ParseBool()
    {
        if (m_Input.substr(m_Pos, 4) == "true") {
            m_Pos += 4;
            return JsonValue{true};
        }
        if (m_Input.substr(m_Pos, 5) == "false") {
            m_Pos += 5;
            return JsonValue{false};
        }
        return JsonValue{};
    }

    JsonValue ParseNumber()
    {
        std::size_t start = m_Pos;
        if (Peek() == '-') ++m_Pos;
        while (!AtEnd() && std::isdigit(static_cast<unsigned char>(Peek()))) ++m_Pos;

        bool isFloat = false;
        if (Peek() == '.') {
            isFloat = true;
            ++m_Pos;
            while (!AtEnd() && std::isdigit(static_cast<unsigned char>(Peek()))) ++m_Pos;
        }
        if (Peek() == 'e' || Peek() == 'E') {
            isFloat = true;
            ++m_Pos;
            if (Peek() == '+' || Peek() == '-') ++m_Pos;
            while (!AtEnd() && std::isdigit(static_cast<unsigned char>(Peek()))) ++m_Pos;
        }

        std::string_view numStr = m_Input.substr(start, m_Pos - start);
        if (isFloat) {
            double val = 0.0;
            std::from_chars(numStr.data(), numStr.data() + numStr.size(), val);
            return JsonValue{val};
        } else {
            int64_t val = 0;
            std::from_chars(numStr.data(), numStr.data() + numStr.size(), val);
            return JsonValue{val};
        }
    }

    std::string ParseRawString()
    {
        Expect('"');
        std::string result;
        while (!AtEnd() && Peek() != '"') {
            char c = Advance();
            if (c == '\\') {
                char esc = Advance();
                switch (esc) {
                    case '"':  result += '"';  break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/';  break;
                    case 'b':  result += '\b'; break;
                    case 'f':  result += '\f'; break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    case 'u': {
                        // Basic 4-hex-digit unicode escape (BMP only).
                        if (m_Pos + 4 > m_Input.size()) break;
                        std::string hex(m_Input.substr(m_Pos, 4));
                        m_Pos += 4;
                        unsigned long cp = std::stoul(hex, nullptr, 16);
                        if (cp < 0x80) {
                            result += static_cast<char>(cp);
                        } else if (cp < 0x800) {
                            result += static_cast<char>(0xC0 | (cp >> 6));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        } else {
                            result += static_cast<char>(0xE0 | (cp >> 12));
                            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        }
                        break;
                    }
                    default: result += esc; break;
                }
            } else {
                result += c;
            }
        }
        Expect('"');
        return result;
    }

    JsonValue ParseString()
    {
        return JsonValue{ParseRawString()};
    }

    JsonValue ParseArray()
    {
        Expect('[');
        SkipWhitespace();
        JsonValue::ArrayType arr;
        if (Peek() != ']') {
            arr.push_back(ParseValue());
            SkipWhitespace();
            while (Match(',')) {
                SkipWhitespace();
                arr.push_back(ParseValue());
                SkipWhitespace();
            }
        }
        Expect(']');
        return JsonValue{std::move(arr)};
    }

    JsonValue ParseObject()
    {
        Expect('{');
        SkipWhitespace();
        JsonValue::ObjectType obj;
        if (Peek() != '}') {
            auto key = ParseRawString();
            SkipWhitespace();
            Expect(':');
            SkipWhitespace();
            obj[std::move(key)] = ParseValue();
            SkipWhitespace();
            while (Match(',')) {
                SkipWhitespace();
                auto k = ParseRawString();
                SkipWhitespace();
                Expect(':');
                SkipWhitespace();
                obj[std::move(k)] = ParseValue();
                SkipWhitespace();
            }
        }
        Expect('}');
        return JsonValue{std::move(obj)};
    }
};

} // anonymous namespace

JsonValue JsonSerializer::DeserializeFromString(std::string_view json)
{
    try {
        Parser parser(json);
        return parser.Parse();
    } catch (...) {
        return JsonValue{};
    }
}

} // namespace Core::Serialization
