#include "Core/Paths/ProjectPathService.h"

#include <sstream>

namespace nf
{
    bool ProjectPathService::InitializeFrom(const std::filesystem::path& startDirectory)
    {
        const auto repoRoot = FindRepoRoot(startDirectory);
        if (!repoRoot.has_value())
        {
            m_repoRoot.clear();
            return false;
        }

        m_repoRoot = std::filesystem::weakly_canonical(*repoRoot);
        return true;
    }

    bool ProjectPathService::IsValid() const noexcept
    {
        return !m_repoRoot.empty();
    }

    const std::filesystem::path& ProjectPathService::GetRepoRoot() const noexcept
    {
        return m_repoRoot;
    }

    std::filesystem::path ProjectPathService::GetManifestPath() const
    {
        return ResolveFromRepo(std::filesystem::path("Config") / "novaforge.project.json");
    }

    std::filesystem::path ProjectPathService::GetContentRoot() const
    {
        return ResolveFromRepo("Content");
    }

    std::filesystem::path ProjectPathService::GetDevWorldPath() const
    {
        return ResolveFromContent(std::filesystem::path("Definitions") / "DevWorld.json");
    }

    std::filesystem::path ProjectPathService::ResolveFromRepo(const std::filesystem::path& relativePath) const
    {
        if (!IsValid())
        {
            return {};
        }

        return m_repoRoot / relativePath;
    }

    std::filesystem::path ProjectPathService::ResolveFromContent(const std::filesystem::path& relativePath) const
    {
        if (!IsValid())
        {
            return {};
        }

        return GetContentRoot() / relativePath;
    }

    std::string ProjectPathService::Describe() const
    {
        std::ostringstream oss;
        oss << "repoRoot=" << (IsValid() ? m_repoRoot.string() : std::string("<invalid>"))
            << " manifest=" << GetManifestPath().string()
            << " contentRoot=" << GetContentRoot().string()
            << " devWorld=" << GetDevWorldPath().string();
        return oss.str();
    }

    std::optional<std::filesystem::path> ProjectPathService::FindRepoRoot(const std::filesystem::path& startDirectory, int maxDepth)
    {
        if (startDirectory.empty())
        {
            return std::nullopt;
        }

        std::filesystem::path cursor = std::filesystem::weakly_canonical(startDirectory);
        for (int depth = 0; depth <= maxDepth; ++depth)
        {
            const auto manifestPath = cursor / "Config" / "novaforge.project.json";
            if (std::filesystem::exists(manifestPath))
            {
                return cursor;
            }

            if (!cursor.has_parent_path())
            {
                break;
            }

            const auto parent = cursor.parent_path();
            if (parent == cursor)
            {
                break;
            }

            cursor = parent;
        }

        return std::nullopt;
    }

    std::filesystem::path ProjectPathService::GuessExecutableDirectory()
    {
        return std::filesystem::current_path();
    }
}
