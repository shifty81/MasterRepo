#pragma once
#include "Engine/World/Level.h"

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

    /// @brief Initialise the game world.
    /// @return True on success.
    bool Initialize();

    /// @brief Advance the world simulation by one variable-rate tick.
    /// @param dt Elapsed seconds since the last frame.
    void Tick(float dt);

    /// @brief Tear down the world and release resources.
    void Shutdown();

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    /// @brief Returns the underlying engine level.
    [[nodiscard]] Level&       GetLevel()       noexcept { return m_Level; }
    /// @copydoc GetLevel()
    [[nodiscard]] const Level& GetLevel() const noexcept { return m_Level; }

    /// @brief Returns true after a successful Initialize() and before Shutdown().
    [[nodiscard]] bool IsReady() const noexcept { return m_Ready; }

private:
    Level m_Level;
    bool  m_Ready{false};
};

} // namespace NF::Game
