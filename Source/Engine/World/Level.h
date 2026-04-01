#pragma once
#include "Engine/ECS/World.h"
#include <filesystem>
#include <string>

namespace NF {

/// @brief Represents a self-contained scene (level) with its own ECS World.
///
/// A Level owns a World instance; entities and components created during Load()
/// are destroyed when Unload() is called.
class Level {
public:
    /// @brief Load level data from the given filesystem path.
    /// @param path Absolute or project-relative path to the level asset file.
    void Load(const std::filesystem::path& path);

    /// @brief Destroy all entities and release level resources.
    void Unload();

    /// @brief Advance level simulation by one variable-rate tick.
    /// @param dt Elapsed seconds since the last frame.
    void Update(float dt);

    /// @brief Access the ECS World that belongs to this level.
    /// @return Reference to the level's World; valid between Load() and Unload().
    [[nodiscard]] World& GetWorld() noexcept { return m_World; }

    /// @copydoc GetWorld()
    [[nodiscard]] const World& GetWorld() const noexcept { return m_World; }

    /// @brief Returns true if the level has been successfully loaded.
    [[nodiscard]] bool IsLoaded() const noexcept { return m_Loaded; }

    /// @brief Returns the path that was supplied to Load().
    [[nodiscard]] const std::filesystem::path& GetPath() const noexcept { return m_Path; }

private:
    World m_World;
    std::filesystem::path m_Path;
    bool m_Loaded{false};
};

} // namespace NF
