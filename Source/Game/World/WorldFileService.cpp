#include "Game/World/WorldFileService.h"

namespace nf
{
    bool WorldFileService::LoadWorld(const std::filesystem::path& path)
    {
        if (path.empty())
        {
            return false;
        }

        m_state.activeWorldPath = path;
        m_state.dirty = false;
        return true;
    }

    bool WorldFileService::SaveWorld()
    {
        if (m_state.activeWorldPath.empty())
        {
            return false;
        }

        m_state.dirty = false;
        return true;
    }

    bool WorldFileService::SaveWorldAs(const std::filesystem::path& path)
    {
        if (path.empty())
        {
            return false;
        }

        m_state.activeWorldPath = path;
        m_state.dirty = false;
        return true;
    }

    bool WorldFileService::ReloadWorld()
    {
        return !m_state.activeWorldPath.empty();
    }

    const WorldFileState& WorldFileService::GetState() const noexcept
    {
        return m_state;
    }

    void WorldFileService::MarkDirty(bool dirty) noexcept
    {
        m_state.dirty = dirty;
    }
}
