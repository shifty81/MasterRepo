#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <cstdint>

namespace atlas::graphvm {

// Simple JSON builder for graph serialization
class JsonBuilder {
public:
    void BeginObject();
    void EndObject();
    void BeginArray(const std::string& key);
    void EndArray();
    void AddString(const std::string& key, const std::string& value);
    void AddInt(const std::string& key, int64_t value);
    void AddFloat(const std::string& key, float value);
    void AddBool(const std::string& key, bool value);
    void BeginArrayObject();
    std::string Build() const;

private:
    std::ostringstream m_ss;
    bool m_needComma = false;
    std::vector<bool> m_commaStack;

    void WriteKey(const std::string& key);
    void PrependComma();
    static std::string EscapeString(const std::string& s);
};

// Simple JSON reader for graph deserialization
class JsonReader {
public:
    bool Parse(const std::string& json);
    std::string GetString(const std::string& key) const;
    int64_t GetInt(const std::string& key) const;
    float GetFloat(const std::string& key) const;
    bool GetBool(const std::string& key) const;
    bool HasKey(const std::string& key) const;

    struct JsonObject {
        std::unordered_map<std::string, std::string> values;
    };

    std::vector<JsonObject> GetArray(const std::string& key) const;

private:
    std::unordered_map<std::string, std::string> m_values;
    std::unordered_map<std::string, std::vector<JsonObject>> m_arrays;

    static std::string UnescapeString(const std::string& s);
    static void SkipWhitespace(const std::string& json, size_t& pos);
    static std::string ParseQuotedString(const std::string& json, size_t& pos);
    static std::string ParseValue(const std::string& json, size_t& pos);
    static JsonObject ParseObject(const std::string& json, size_t& pos);
    static std::vector<JsonObject> ParseArray(const std::string& json, size_t& pos);
};

} // namespace atlas::graphvm
