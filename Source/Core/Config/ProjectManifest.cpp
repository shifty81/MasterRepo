#include "Core/Config/ProjectManifest.h"
#include "Core/Logging/Log.h"
#include <fstream>
#include <sstream>

namespace NF {

// ============================================================================
// Minimal JSON helpers (handles the flat structure of novaforge.project.json)
// ============================================================================

namespace {

/// @brief Trim leading/trailing whitespace and quotes from a value string.
std::string TrimValue(const std::string& raw)
{
    std::string s = raw;
    // Strip leading whitespace
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    s = s.substr(start);

    // Strip trailing whitespace and commas
    auto end = s.find_last_not_of(" \t\r\n,");
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
        if (line.find("\"phases\"") != std::string::npos)
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
            auto v = ExtractStringValue(line, "name");
            if (!v.empty()) ProjectName = v;
        }
        {
            auto v = ExtractStringValue(line, "type");
            if (!v.empty()) ProjectType = v;
        }
        {
            auto v = ExtractStringValue(line, "version");
            if (!v.empty()) ProjectVersion = v;
        }
        {
            auto v = ExtractStringValue(line, "defaultWorld");
            if (!v.empty()) DefaultWorld = v;
        }
        {
            auto v = ExtractStringValue(line, "defaultStartupMode");
            if (!v.empty()) DefaultStartupMode = v;
        }

        // paths block
        {
            auto v = ExtractStringValue(line, "contentRoot");
            if (!v.empty()) ContentRoot = v;
        }
        {
            auto v = ExtractStringValue(line, "configRoot");
            if (!v.empty()) ConfigRoot = v;
        }
        {
            auto v = ExtractStringValue(line, "saveRoot");
            if (!v.empty()) SaveRoot = v;
        }
        {
            auto v = ExtractStringValue(line, "logRoot");
            if (!v.empty()) LogRoot = v;
        }

        // rules block
        if (line.find("\"voxelAuthoritative\"") != std::string::npos)
            VoxelAuthoritative = ExtractBoolValue(line, "voxelAuthoritative", true);
        if (line.find("\"editorShipsWithGame\"") != std::string::npos)
            EditorShipsWithGame = ExtractBoolValue(line, "editorShipsWithGame", false);
        if (line.find("\"allowSuiteFeaturesInRepo\"") != std::string::npos)
            AllowSuiteFeaturesInRepo = ExtractBoolValue(line, "allowSuiteFeaturesInRepo", false);
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
