#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace nf
{
    struct DevWorldData
    {
        std::string worldName = "DevWorld";
        int seed = 1337;
        int initialChunkRadius = 2;
    };

    class DevWorldSerializer
    {
    public:
        std::optional<DevWorldData> Load(const std::filesystem::path& path) const;
        bool Save(const std::filesystem::path& path, const DevWorldData& data) const;
    };
}
