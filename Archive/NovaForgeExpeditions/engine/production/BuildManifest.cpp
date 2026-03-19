#include "BuildManifest.h"
#include <sstream>

namespace atlas::production {

// --- JSON helpers (matches FlowGraphIR / GraphSerializer pattern) ---

static std::string EscapeString(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

static std::string UnescapeString(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char next = s[i + 1];
            switch (next) {
                case '"':  out += '"';  ++i; break;
                case '\\': out += '\\'; ++i; break;
                case 'n':  out += '\n'; ++i; break;
                case 'r':  out += '\r'; ++i; break;
                case 't':  out += '\t'; ++i; break;
                default:   out += s[i]; break;
            }
        } else {
            out += s[i];
        }
    }
    return out;
}

static void SkipWhitespace(const std::string& json, size_t& pos) {
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
           json[pos] == '\n' || json[pos] == '\r')) {
        ++pos;
    }
}

static std::string ParseQuotedString(const std::string& json, size_t& pos) {
    if (pos >= json.size() || json[pos] != '"') return "";
    ++pos;
    std::string result;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            result += json[pos];
            result += json[pos + 1];
            pos += 2;
        } else {
            result += json[pos];
            ++pos;
        }
    }
    if (pos < json.size()) ++pos;
    return UnescapeString(result);
}

static std::string ParseValue(const std::string& json, size_t& pos) {
    SkipWhitespace(json, pos);
    if (pos >= json.size()) return "";

    if (json[pos] == '"') {
        return ParseQuotedString(json, pos);
    }

    std::string result;
    while (pos < json.size() && json[pos] != ',' && json[pos] != '}' &&
           json[pos] != ']' && json[pos] != ' ' && json[pos] != '\n' &&
           json[pos] != '\r' && json[pos] != '\t') {
        result += json[pos];
        ++pos;
    }
    return result;
}

static void SkipNestedStructure(const std::string& json, size_t& pos) {
    char open = json[pos];
    char close = (open == '[') ? ']' : '}';
    int depth = 1;
    ++pos;
    while (pos < json.size() && depth > 0) {
        if (json[pos] == open) ++depth;
        else if (json[pos] == close) --depth;
        else if (json[pos] == '"') {
            ++pos;
            while (pos < json.size() && json[pos] != '"') {
                if (json[pos] == '\\') ++pos;
                ++pos;
            }
        }
        ++pos;
    }
}

// --- BuildManifest::ToJSON ---

std::string BuildManifest::ToJSON() const {
    std::ostringstream ss;
    ss << "{";
    ss << "\"schemaVersion\":" << schemaVersion;
    ss << ",\"projectName\":\"" << EscapeString(projectName) << "\"";
    ss << ",\"buildType\":\"" << EscapeString(buildType) << "\"";
    ss << ",\"platform\":\"" << EscapeString(platform) << "\"";
    ss << ",\"engineVersion\":\"" << EscapeString(engineVersion) << "\"";
    ss << ",\"buildTimestamp\":\"" << EscapeString(buildTimestamp) << "\"";
    ss << ",\"layoutHash\":" << layoutHash;
    ss << ",\"dslVersion\":\"" << EscapeString(dslVersion) << "\"";

    // Artifacts
    ss << ",\"artifacts\":[";
    for (size_t i = 0; i < artifacts.size(); ++i) {
        if (i > 0) ss << ",";
        const auto& a = artifacts[i];
        ss << "{";
        ss << "\"path\":\"" << EscapeString(a.path) << "\"";
        ss << ",\"type\":\"" << EscapeString(a.type) << "\"";
        ss << ",\"hash\":" << a.hash;
        ss << ",\"size\":" << a.size;
        ss << ",\"version\":\"" << EscapeString(a.version) << "\"";
        ss << "}";
    }
    ss << "]";

    // Metadata
    ss << ",\"metadata\":{";
    bool first = true;
    for (const auto& kv : metadata) {
        if (!first) ss << ",";
        ss << "\"" << EscapeString(kv.first) << "\":\"" << EscapeString(kv.second) << "\"";
        first = false;
    }
    ss << "}";

    ss << "}";
    return ss.str();
}

// --- BuildManifest::FromJSON ---

static ArtifactEntry ParseArtifactObject(const std::string& json, size_t& pos) {
    ArtifactEntry entry;
    SkipWhitespace(json, pos);
    if (pos >= json.size() || json[pos] != '{') return entry;
    ++pos;

    while (pos < json.size()) {
        SkipWhitespace(json, pos);
        if (pos >= json.size() || json[pos] == '}') { ++pos; break; }
        if (json[pos] == ',') { ++pos; continue; }

        std::string key = ParseQuotedString(json, pos);
        SkipWhitespace(json, pos);
        if (pos < json.size() && json[pos] == ':') ++pos;
        SkipWhitespace(json, pos);

        if (pos < json.size() && (json[pos] == '[' || json[pos] == '{')) {
            SkipNestedStructure(json, pos);
        } else {
            std::string value = ParseValue(json, pos);
            try {
                if (key == "path") entry.path = value;
                else if (key == "type") entry.type = value;
                else if (key == "hash") entry.hash = std::stoull(value);
                else if (key == "size") entry.size = std::stoull(value);
                else if (key == "version") entry.version = value;
            } catch (...) {}
        }
    }
    return entry;
}

static void ParseMetadataObject(const std::string& json, size_t& pos,
                                std::unordered_map<std::string, std::string>& metadata) {
    SkipWhitespace(json, pos);
    if (pos >= json.size() || json[pos] != '{') return;
    ++pos;

    while (pos < json.size()) {
        SkipWhitespace(json, pos);
        if (pos >= json.size() || json[pos] == '}') { ++pos; break; }
        if (json[pos] == ',') { ++pos; continue; }

        std::string key = ParseQuotedString(json, pos);
        SkipWhitespace(json, pos);
        if (pos < json.size() && json[pos] == ':') ++pos;
        SkipWhitespace(json, pos);

        std::string value = ParseValue(json, pos);
        metadata[key] = value;
    }
}

BuildManifest BuildManifest::FromJSON(const std::string& json) {
    BuildManifest manifest;
    size_t pos = 0;
    SkipWhitespace(json, pos);
    if (pos >= json.size() || json[pos] != '{') return manifest;
    ++pos;

    while (pos < json.size()) {
        SkipWhitespace(json, pos);
        if (pos >= json.size() || json[pos] == '}') break;
        if (json[pos] == ',') { ++pos; continue; }

        std::string key = ParseQuotedString(json, pos);
        SkipWhitespace(json, pos);
        if (pos < json.size() && json[pos] == ':') ++pos;
        SkipWhitespace(json, pos);

        if (key == "artifacts" && pos < json.size() && json[pos] == '[') {
            ++pos;
            while (pos < json.size()) {
                SkipWhitespace(json, pos);
                if (pos >= json.size() || json[pos] == ']') { ++pos; break; }
                if (json[pos] == ',') { ++pos; continue; }
                if (json[pos] == '{') {
                    manifest.artifacts.push_back(ParseArtifactObject(json, pos));
                }
            }
        } else if (key == "metadata" && pos < json.size() && json[pos] == '{') {
            ParseMetadataObject(json, pos, manifest.metadata);
        } else if (pos < json.size() && (json[pos] == '[' || json[pos] == '{')) {
            SkipNestedStructure(json, pos);
        } else {
            std::string value = ParseValue(json, pos);
            try {
                if (key == "schemaVersion") manifest.schemaVersion = static_cast<uint32_t>(std::stoul(value));
                else if (key == "projectName") manifest.projectName = value;
                else if (key == "buildType") manifest.buildType = value;
                else if (key == "platform") manifest.platform = value;
                else if (key == "engineVersion") manifest.engineVersion = value;
                else if (key == "buildTimestamp") manifest.buildTimestamp = value;
                else if (key == "layoutHash") manifest.layoutHash = std::stoull(value);
                else if (key == "dslVersion") manifest.dslVersion = value;
            } catch (...) {}
        }
    }
    return manifest;
}

// --- Validation ---

bool BuildManifest::Validate() const {
    if (schemaVersion != SCHEMA_VERSION) return false;
    if (projectName.empty()) return false;
    if (buildType.empty()) return false;
    if (platform.empty()) return false;

    // Validate buildType values
    if (buildType != "Debug" && buildType != "Development" && buildType != "Release")
        return false;

    // Validate platform values
    if (platform != "Windows" && platform != "Linux" && platform != "macOS" && platform != "Web")
        return false;

    // Validate artifacts
    for (const auto& a : artifacts) {
        if (a.path.empty()) return false;
        if (a.type.empty()) return false;
        if (a.type != "executable" && a.type != "library" &&
            a.type != "asset" && a.type != "config")
            return false;
    }

    return true;
}

// --- Artifact management ---

void BuildManifest::AddArtifact(const ArtifactEntry& entry) {
    artifacts.push_back(entry);
}

const ArtifactEntry* BuildManifest::FindArtifact(const std::string& path) const {
    for (const auto& a : artifacts) {
        if (a.path == path) return &a;
    }
    return nullptr;
}

bool BuildManifest::VerifyArtifactHash(const std::string& path, uint64_t expectedHash) const {
    const ArtifactEntry* entry = FindArtifact(path);
    if (!entry) return false;
    return entry->hash == expectedHash;
}

} // namespace atlas::production
