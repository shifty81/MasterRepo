#pragma once
#include "Engine/World/Level.h"
#include "Renderer/RHI/RenderDevice.h"
#include "Game/World/GameWorld.h"
#include "Game/Interaction/InteractionLoop.h"
#include "Game/Movement/PlayerMovement.h"
#include <memory>

namespace NF::Game {

/// @brief Top-level game runtime orchestrator.
///
/// Owns the active @c Level, the game world (voxel + ECS), the Phase 3
/// interaction loop, and the Phase 5 player movement controller.
/// Holds a non-owning pointer to the @c RenderDevice provided at Init()
/// time.  Call Init(), then loop Tick(), then Shutdown().
class Orchestrator {
public:
    Orchestrator() = default;
    ~Orchestrator() = default;

    Orchestrator(const Orchestrator&)            = delete;
    Orchestrator& operator=(const Orchestrator&) = delete;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// @brief Initialise all runtime subsystems.
    /// @param renderDevice Non-owning pointer to the render device; must
    ///        outlive this Orchestrator.
    /// @return True on success.
    bool Init(RenderDevice* renderDevice);

    /// @brief Advance the game simulation by one variable-rate tick.
    /// @param dt Elapsed seconds since the last frame.
    void Tick(float dt);

    /// @brief Tear down all runtime subsystems.
    void Shutdown();

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    /// @brief Returns a pointer to the active level, or nullptr before Init().
    [[nodiscard]] Level*       GetLevel()       noexcept { return &m_Level; }
    /// @copydoc GetLevel()
    [[nodiscard]] const Level* GetLevel() const noexcept { return &m_Level; }

    /// @brief Returns the game world facade (voxel + ECS + dev-world config).
    [[nodiscard]] GameWorld&       GetGameWorld()       noexcept { return m_GameWorld; }
    [[nodiscard]] const GameWorld& GetGameWorld() const noexcept { return m_GameWorld; }

    /// @brief Returns the Phase 3 first interaction loop.
    [[nodiscard]] InteractionLoop&       GetInteractionLoop()       noexcept { return m_InteractionLoop; }
    [[nodiscard]] const InteractionLoop& GetInteractionLoop() const noexcept { return m_InteractionLoop; }

    /// @brief Returns the Phase 5 player movement controller.
    [[nodiscard]] PlayerMovement&       GetPlayerMovement()       noexcept { return m_PlayerMovement; }
    [[nodiscard]] const PlayerMovement& GetPlayerMovement() const noexcept { return m_PlayerMovement; }

    /// @brief Returns true after a successful Init() and before Shutdown().
    [[nodiscard]] bool IsInitialized() const noexcept { return m_Initialized; }

private:
    Level            m_Level;
    GameWorld        m_GameWorld;
    InteractionLoop  m_InteractionLoop;
    PlayerMovement   m_PlayerMovement;
    RenderDevice*    m_RenderDevice{nullptr};
    bool             m_Initialized{false};
};

} // namespace NF::Game
