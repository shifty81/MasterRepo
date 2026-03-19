#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace Runtime::SaveLoad {

struct SaveHeader {
    std::string version   = "1.0";
    std::string timestamp;
    std::string sceneName;
    uint64_t    saveIndex = 0;
};

struct SaveData {
    SaveHeader  header;
    std::string jsonPayload; // serialized game state
};

class SaveSystem {
public:
    // Write `data` to `filePath`.  Returns true on success.
    bool Save(const SaveData& data, const std::string& filePath);

    // Read a previously saved file into `outData`.  Returns true on success.
    bool Load(const std::string& filePath, SaveData& outData);

    // Write to `saveDir/quicksave.sav`.
    bool QuickSave(const SaveData& data, const std::string& saveDir);

    // Read from `saveDir/quicksave.sav`.
    bool QuickLoad(const std::string& saveDir, SaveData& outData);

    // Return the paths of all *.sav files in `saveDir`.
    std::vector<std::string> ListSaves(const std::string& saveDir) const;

    // Delete the file at `filePath`.  Returns true on success.
    bool DeleteSave(const std::string& filePath);

private:
    static constexpr const char* kQuickSaveName = "quicksave.sav";
};

} // namespace Runtime::SaveLoad
