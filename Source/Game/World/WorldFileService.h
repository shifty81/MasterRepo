#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace nf
{
    struct WorldFileState
    {
        std::filesystem::path activeWorldPath;
        bool dirty = false;
    };

    class WorldFileService
    {
    public:
        bool LoadWorld(const std::filesystem::path& path);
        bool SaveWorld();
        bool SaveWorldAs(const std::filesystem::path& path);
        bool ReloadWorld();

        const WorldFileState& GetState() const noexcept;
        void MarkDirty(bool dirty = true) noexcept;

    private:
        WorldFileState m_state;
    };
}
