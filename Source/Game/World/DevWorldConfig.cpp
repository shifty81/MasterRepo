#include "Game/World/DevWorldConfig.h"
#include "Core/Logging/Log.h"
#include <fstream>
#include <sstream>
#include <string>

namespace NF::Game {

// ---------------------------------------------------------------------------
// Minimal JSON helpers (no external dependency)
// ---------------------------------------------------------------------------

namespace {

/// @brief Trim whitespace from both ends of a string.
std::string Trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    auto end   = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

/// @brief Extract the string value of a key from a flat-ish JSON blob.
std::string JsonString(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return {};
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return {};
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return {};
    auto end = json.find('"', pos + 1);
    if (end == std::string::npos) return {};
    return json.substr(pos + 1, end - pos - 1);
}

/// @brief Extract a numeric value of a key from a flat-ish JSON blob.
float JsonFloat(const std::string& json, const std::string& key, float fallback) {
    const std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return fallback;
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return fallback;
    // Skip whitespace after colon.
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    try { return std::stof(json.substr(pos)); }
    catch (...) { return fallback; }
}

/// @brief Extract a float array [a, b, c] following a key.
bool JsonFloatArray(const std::string& json, const std::string& key,
                    float* out, size_t count) {
    const std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return false;
    pos = json.find('[', pos + needle.size());
    if (pos == std::string::npos) return false;
    ++pos; // skip '['
    for (size_t i = 0; i < count; ++i) {
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'
               || json[pos] == '\r' || json[pos] == '\n')) ++pos;
        try {
            size_t consumed = 0;
            out[i] = std::stof(json.substr(pos), &consumed);
            pos += consumed;
        } catch (...) {
            return false;
        }
        // Skip comma / whitespace.
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == ','
               || json[pos] == '\t')) ++pos;
    }
    return true;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// LoadFromFile
// ---------------------------------------------------------------------------

bool DevWorldConfig::LoadFromFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::Log(LogLevel::Warning, "DevWorld",
                    "Cannot open DevWorld config: " + path);
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string json = ss.str();

    // Identity
    {
        auto val = JsonString(json, "id");
        if (!val.empty()) m_WorldId = val;
    }
    {
        auto val = JsonString(json, "displayName");
        if (!val.empty()) m_DisplayName = val;
    }
    {
        auto val = JsonString(json, "description");
        if (!val.empty()) m_Description = val;
    }

    // World parameters
    m_Seed    = static_cast<uint32_t>(JsonFloat(json, "seed", static_cast<float>(m_Seed)));
    m_Gravity = JsonFloat(json, "gravity", m_Gravity);

    {
        float ts[3]{};
        if (JsonFloatArray(json, "terrainSize", ts, 3))
            m_TerrainSize = {ts[0], ts[1], ts[2]};
    }

    // Spawn
    {
        float pos[3]{};
        if (JsonFloatArray(json, "position", pos, 3))
            m_Spawn.Position = {pos[0], pos[1], pos[2]};
    }
    {
        float rot[4]{};
        if (JsonFloatArray(json, "rotation", rot, 4))
            m_Spawn.Rotation = {rot[0], rot[1], rot[2], rot[3]};
    }
    {
        float dir[3]{};
        if (JsonFloatArray(json, "lookDirection", dir, 3))
            m_Spawn.LookDirection = {dir[0], dir[1], dir[2]};
    }

    // Camera
    m_Camera.FOV             = JsonFloat(json, "fov", m_Camera.FOV);
    m_Camera.NearClip        = JsonFloat(json, "nearClip", m_Camera.NearClip);
    m_Camera.FarClip         = JsonFloat(json, "farClip", m_Camera.FarClip);
    m_Camera.MoveSpeed       = JsonFloat(json, "moveSpeed", m_Camera.MoveSpeed);
    m_Camera.LookSensitivity = JsonFloat(json, "lookSensitivity", m_Camera.LookSensitivity);

    m_Valid = true;
    Logger::Log(LogLevel::Info, "DevWorld",
                "Loaded DevWorld config: " + m_DisplayName
                + " (seed=" + std::to_string(m_Seed) + ")");
    return true;
}

// ---------------------------------------------------------------------------
// Defaults
// ---------------------------------------------------------------------------

DevWorldConfig DevWorldConfig::Defaults()
{
    DevWorldConfig cfg;
    cfg.m_Valid = true;
    Logger::Log(LogLevel::Info, "DevWorld", "Using compiled-in DevWorld defaults");
    return cfg;
}

} // namespace NF::Game
