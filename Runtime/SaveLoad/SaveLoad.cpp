#include "Runtime/SaveLoad/SaveLoad.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>

namespace Runtime::SaveLoad {

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string CurrentTimestamp() {
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char buf[32]{};
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", std::gmtime(&time));
    return buf;
}

// Simple text format:
//   Line 0: version
//   Line 1: timestamp
//   Line 2: sceneName
//   Line 3: saveIndex
//   Remaining lines: jsonPayload
static bool WriteToStream(std::ostream& out, const SaveData& data) {
    out << data.header.version   << '\n'
        << data.header.timestamp << '\n'
        << data.header.sceneName << '\n'
        << data.header.saveIndex << '\n'
        << data.jsonPayload;
    return out.good();
}

static bool ReadFromStream(std::istream& in, SaveData& outData) {
    std::string versionLine, timestampLine, sceneNameLine, saveIndexLine;
    if (!std::getline(in, versionLine))   return false;
    if (!std::getline(in, timestampLine)) return false;
    if (!std::getline(in, sceneNameLine)) return false;
    if (!std::getline(in, saveIndexLine)) return false;

    outData.header.version   = versionLine;
    outData.header.timestamp = timestampLine;
    outData.header.sceneName = sceneNameLine;
    try {
        outData.header.saveIndex = static_cast<uint64_t>(std::stoull(saveIndexLine));
    } catch (...) {
        outData.header.saveIndex = 0;
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    outData.jsonPayload = ss.str();
    return true;
}

// ---------------------------------------------------------------------------
// SaveSystem
// ---------------------------------------------------------------------------
bool SaveSystem::Save(const SaveData& data, const std::string& filePath) {
    SaveData mutable_data = data;
    if (mutable_data.header.timestamp.empty())
        mutable_data.header.timestamp = CurrentTimestamp();

    std::ofstream out(filePath, std::ios::out | std::ios::trunc);
    if (!out.is_open())
        return false;
    return WriteToStream(out, mutable_data);
}

bool SaveSystem::Load(const std::string& filePath, SaveData& outData) {
    std::ifstream in(filePath);
    if (!in.is_open())
        return false;
    return ReadFromStream(in, outData);
}

bool SaveSystem::QuickSave(const SaveData& data, const std::string& saveDir) {
    fs::path dir(saveDir);
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) return false;
    return Save(data, (dir / kQuickSaveName).string());
}

bool SaveSystem::QuickLoad(const std::string& saveDir, SaveData& outData) {
    fs::path path = fs::path(saveDir) / kQuickSaveName;
    return Load(path.string(), outData);
}

std::vector<std::string> SaveSystem::ListSaves(const std::string& saveDir) const {
    std::vector<std::string> result;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(saveDir, ec)) {
        if (entry.is_regular_file(ec) &&
            entry.path().extension() == ".sav") {
            result.push_back(entry.path().string());
        }
    }
    return result;
}

bool SaveSystem::DeleteSave(const std::string& filePath) {
    std::error_code ec;
    return fs::remove(filePath, ec) && !ec;
}

} // namespace Runtime::SaveLoad
