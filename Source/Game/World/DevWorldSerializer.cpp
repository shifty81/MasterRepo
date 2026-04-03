#include "Game/World/DevWorldSerializer.h"

#include <fstream>

namespace nf
{
    std::optional<DevWorldData> DevWorldSerializer::Load(const std::filesystem::path& path) const
    {
        if (path.empty())
        {
            return std::nullopt;
        }

        std::ifstream in(path);
        if (!in.is_open())
        {
            return std::nullopt;
        }

        // Scaffold only.
        // Replace with strict JSON parse in real integration.
        DevWorldData data {};
        return data;
    }

    bool DevWorldSerializer::Save(const std::filesystem::path& path, const DevWorldData& data) const
    {
        if (path.empty())
        {
            return false;
        }

        std::ofstream out(path);
        if (!out.is_open())
        {
            return false;
        }

        out << "{\n";
        out << "  \"worldName\": \"" << data.worldName << "\",\n";
        out << "  \"seed\": " << data.seed << ",\n";
        out << "  \"initialChunkRadius\": " << data.initialChunkRadius << "\n";
        out << "}\n";

        return true;
    }
}
