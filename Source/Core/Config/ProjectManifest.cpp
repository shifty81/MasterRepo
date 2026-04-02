#include "Core/Config/ProjectManifest.h"
#include "Core/Logging/Log.h"
#include <fstream>
#include <sstream>

namespace NF {

// ============================================================================
// Minimal JSON helpers (handles the flat structure of novaforge.project.json)
// ============================================================================

namespace {

constexpr const char* kWhitespace = " \t\r\n";

/// @brief Trim leading/trailing whitespace and quotes from a value string.
std::string TrimValue(const std::string& raw)
{
    std::string s = raw;
    // Strip leading whitespace
    auto start = s.find_first_not_of(kWhitespace);
    if (start == std::string::npos) return {};
    s = s.substr(start);

    // Strip trailing whitespace and commas
    std::string trailChars = std::string(kWhitespace) + ",";
    auto end = s.find_last_not_of(trailChars);
    if (end != std::string::npos)
        s = s.substr(0, end + 1);

    // Strip surrounding quotes
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        s = s.substr(1, s.size() - 2);

    return s;
}

/// @brief Extract the value for a given JSON key from a "key": "value" line.
/// @return Empty string when the key is not found.
std::string ExtractStringValue(const std::string& line, const std::string& key)
{
    const std::string pattern = "\"" + key + "\"";
    auto pos = line.find(pattern);
    if (pos == std::string::npos) return {};

    auto colon = line.find(':', pos + pattern.size());
    if (colon == std::string::npos) return {};

    return TrimValue(line.substr(colon + 1));
}

/// @brief Extract a boolean value for a given JSON key.
bool ExtractBoolValue(const std::string& line, const std::string& key, bool defaultVal)
{
    const std::string pattern = "\"" + key + "\"";
    auto pos = line.find(pattern);
    if (pos == std::string::npos) return defaultVal;

    auto colon = line.find(':', pos + pattern.size());
    if (colon == std::string::npos) return defaultVal;

    std::string val = TrimValue(line.substr(colon + 1));
    if (val == "true")  return true;
    if (val == "false") return false;
    return defaultVal;
}

// JSON key constants
constexpr const char* kKeyPhases              = "phases";
constexpr const char* kKeyName                = "name";
constexpr const char* kKeyType                = "type";
constexpr const char* kKeyVersion             = "version";
constexpr const char* kKeyDefaultWorld        = "defaultWorld";
constexpr const char* kKeyDefaultStartupMode  = "defaultStartupMode";
constexpr const char* kKeyContentRoot         = "contentRoot";
constexpr const char* kKeyConfigRoot          = "configRoot";
constexpr const char* kKeySaveRoot            = "saveRoot";
constexpr const char* kKeyLogRoot             = "logRoot";
constexpr const char* kKeyVoxelAuth           = "voxelAuthoritative";
constexpr const char* kKeyEditorShips         = "editorShipsWithGame";
constexpr const char* kKeyAllowSuite          = "allowSuiteFeaturesInRepo";

} // anonymous namespace

// ============================================================================
// ProjectManifest
// ============================================================================

bool ProjectManifest::LoadFromFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        Logger::Log(LogLevel::Error, "Manifest",
                    "Failed to open project manifest: " + path);
        return false;
    }

    // Read the entire file
    std::ostringstream buf;
    buf << file.rdbuf();
    const std::string content = buf.str();

    // Parse line by line for the expected keys
    std::istringstream stream(content);
    std::string line;
    bool inPhases = false;

    Phases.clear();

    while (std::getline(stream, line))
    {
        // Detect array boundaries
        if (line.find(std::string("\"") + kKeyPhases + "\"") != std::string::npos)
        {
            inPhases = true;
            continue;
        }
        if (inPhases)
        {
            if (line.find(']') != std::string::npos)
            {
                inPhases = false;
                continue;
            }
            std::string val = TrimValue(line);
            if (!val.empty())
                Phases.push_back(val);
            continue;
        }

        // project block
        {
            auto v = ExtractStringValue(line, kKeyName);
            if (!v.empty()) ProjectName = v;
        }
        {
            auto v = ExtractStringValue(line, kKeyType);
            if (!v.empty()) ProjectType = v;
        }
        {
            auto v = ExtractStringValue(line, kKeyVersion);
            if (!v.empty()) ProjectVersion = v;
        }
        {
            auto v = ExtractStringValue(line, kKeyDefaultWorld);
            if (!v.empty()) DefaultWorld = v;
        }
        {
            auto v = ExtractStringValue(line, kKeyDefaultStartupMode);
            if (!v.empty()) DefaultStartupMode = v;
        }

        // paths block
        {
            auto v = ExtractStringValue(line, kKeyContentRoot);
            if (!v.empty()) ContentRoot = v;
        }
        {
            auto v = ExtractStringValue(line, kKeyConfigRoot);
            if (!v.empty()) ConfigRoot = v;
        }
        {
            auto v = ExtractStringValue(line, kKeySaveRoot);
            if (!v.empty()) SaveRoot = v;
        }
        {
            auto v = ExtractStringValue(line, kKeyLogRoot);
            if (!v.empty()) LogRoot = v;
        }

        // rules block
        if (line.find(std::string("\"") + kKeyVoxelAuth + "\"") != std::string::npos)
            VoxelAuthoritative = ExtractBoolValue(line, kKeyVoxelAuth, true);
        if (line.find(std::string("\"") + kKeyEditorShips + "\"") != std::string::npos)
            EditorShipsWithGame = ExtractBoolValue(line, kKeyEditorShips, false);
        if (line.find(std::string("\"") + kKeyAllowSuite + "\"") != std::string::npos)
            AllowSuiteFeaturesInRepo = ExtractBoolValue(line, kKeyAllowSuite, false);
    }

    Logger::Log(LogLevel::Info, "Manifest",
                "Loaded project manifest from " + path);
    return true;
}

bool ProjectManifest::IsValid() const noexcept
{
    return !ProjectName.empty() && !ProjectVersion.empty();
}

void ProjectManifest::LogSummary() const
{
    Logger::Log(LogLevel::Info, "Manifest",
                "  Project:  " + ProjectName + " v" + ProjectVersion);
    Logger::Log(LogLevel::Info, "Manifest",
                "  Type:     " + ProjectType);
    Logger::Log(LogLevel::Info, "Manifest",
                "  World:    " + DefaultWorld);
    Logger::Log(LogLevel::Info, "Manifest",
                "  Startup:  " + DefaultStartupMode);
    Logger::Log(LogLevel::Info, "Manifest",
                "  Content:  " + ContentRoot);
    Logger::Log(LogLevel::Info, "Manifest",
                "  Config:   " + ConfigRoot);
    Logger::Log(LogLevel::Info, "Manifest",
                "  Save:     " + SaveRoot);
    Logger::Log(LogLevel::Info, "Manifest",
                "  Logs:     " + LogRoot);

    std::string phaseList;
    for (size_t i = 0; i < Phases.size(); ++i)
    {
        if (i > 0) phaseList += ", ";
        phaseList += Phases[i];
    }
    Logger::Log(LogLevel::Info, "Manifest",
                "  Phases:   " + phaseList);

    Logger::Log(LogLevel::Info, "Manifest",
                std::string("  Voxel authoritative: ")
                + (VoxelAuthoritative ? "yes" : "no"));
}

} // namespace NF
