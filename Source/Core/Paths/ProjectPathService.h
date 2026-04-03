#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace nf
{
    class ProjectPathService
    {
    public:
        // Initializes the resolver from a known start directory, usually the executable directory.
        bool InitializeFrom(const std::filesystem::path& startDirectory);

        // True when the repo root and manifest were found.
        bool IsValid() const noexcept;

        // Repo root directory that contains Config/novaforge.project.json.
        const std::filesystem::path& GetRepoRoot() const noexcept;

        // Full path to Config/novaforge.project.json.
        std::filesystem::path GetManifestPath() const;

        // Full path to repo-root/Content.
        std::filesystem::path GetContentRoot() const;

        // Full path to repo-root/Content/Definitions/DevWorld.json.
        std::filesystem::path GetDevWorldPath() const;

        // Resolve a path relative to repo root.
        std::filesystem::path ResolveFromRepo(const std::filesystem::path& relativePath) const;

        // Resolve a path relative to Content.
        std::filesystem::path ResolveFromContent(const std::filesystem::path& relativePath) const;

        // Human-readable status line for logging.
        std::string Describe() const;

        // Static helpers.
        static std::optional<std::filesystem::path> FindRepoRoot(const std::filesystem::path& startDirectory, int maxDepth = 10);
        static std::filesystem::path GuessExecutableDirectory();

    private:
        std::filesystem::path m_repoRoot;
    };
}
