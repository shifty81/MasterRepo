#pragma once
#include "Engine/World/Level.h"
#include "Game/World/DevWorldConfig.h"
#include "Game/World/WorldDebugOverlay.h"
#include "Game/World/WorldSaveLoad.h"
#include "Engine/ECS/World.h"

namespace NF::Game {

/// @brief Game-layer world facade built on top of @c NF::Level.
///
/// Wraps the engine's @c Level with game-specific initialization logic
/// (e.g. spawning world entities, loading procedural data).  All ECS
/// access is still performed through @c GetLevel().GetWorld().
class GameWorld {
public:
    GameWorld() = default;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// @brief Initialise the game world using the dev world configuration.
    /// @param contentRoot Project-relative or absolute path to the Content directory.
    /// @return True on success.
    bool Initialize(const std::string& contentRoot = "Content");

    /// @brief Advance the world simulation by one variable-rate tick.
    /// @param dt Elapsed seconds since the last frame.
    void Tick(float dt);

    /// @brief Tear down the world and release resources.
    void Shutdown();

    // -------------------------------------------------------------------------
    // Save / Load
    // -------------------------------------------------------------------------

    /// @brief Save current world state to disk.
    /// @param path File path for the save file.
    /// @return True on success.
    bool SaveWorld(const std::string& path);

    /// @brief Load world state from disk.
    /// @param path File path for the save file.
    /// @return True on success.
    bool LoadWorld(const std::string& path);

    // -------------------------------------------------------------------------
    // Debug
    // -------------------------------------------------------------------------

    /// @brief Capture and log the debug overlay to the console.
    void LogDebugOverlay();

    /// @brief Access the debug overlay.
    [[nodiscard]] WorldDebugOverlay& GetDebugOverlay() noexcept { return m_DebugOverlay; }

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    /// @brief Returns the underlying engine level.
    [[nodiscard]] Level&       GetLevel()       noexcept { return m_Level; }
    /// @copydoc GetLevel()
    [[nodiscard]] const Level& GetLevel() const noexcept { return m_Level; }

    /// @brief Returns the dev world configuration.
    [[nodiscard]] const DevWorldConfig& GetConfig() const noexcept { return m_Config; }

    /// @brief Returns the player entity id, or NullEntity if none.
    [[nodiscard]] EntityId GetPlayerEntity() const noexcept { return m_PlayerEntity; }

    /// @brief Returns the spawn point from the dev world config.
    [[nodiscard]] const SpawnPoint& GetSpawnPoint() const noexcept {
        return m_Config.GetSpawnPoint();
    }

    /// @brief Returns true after a successful Initialize() and before Shutdown().
    [[nodiscard]] bool IsReady() const noexcept { return m_Ready; }

private:
    Level             m_Level;
    DevWorldConfig    m_Config;
    WorldDebugOverlay m_DebugOverlay;
    WorldSaveLoad     m_SaveLoad;
    EntityId          m_PlayerEntity{NullEntity};
    bool              m_Ready{false};
};

} // namespace NF::Game
