#pragma once
#include "Game/World/DevWorldConfig.h"
#include "Engine/ECS/World.h"
#include <string>
#include <vector>

namespace NF::Game {

/// @brief Diagnostic overlay that logs world state for debugging.
///
/// Collects snapshot data from the active DevWorldConfig and ECS World,
/// then exposes it as structured text lines.  In a full rendering pipeline
/// these lines would be drawn on-screen; for Phase 1 the overlay logs to
/// the console.
class WorldDebugOverlay {
public:
    WorldDebugOverlay() = default;

    /// @brief Capture a snapshot of current world state.
    /// @param config The active dev world configuration.
    /// @param world  The ECS world to inspect (may be nullptr).
    /// @param playerEntityId  Entity id for the player, or NullEntity.
    void Update(const DevWorldConfig& config, const World* world,
                EntityId playerEntityId);

    /// @brief Log all overlay lines to the engine console.
    void LogToConsole() const;

    /// @brief Return the formatted overlay lines (one string per line).
    [[nodiscard]] const std::vector<std::string>& GetLines() const noexcept {
        return m_Lines;
    }

    /// @brief Controls whether the overlay is active.
    void SetEnabled(bool enabled) noexcept { m_Enabled = enabled; }
    [[nodiscard]] bool IsEnabled() const noexcept { return m_Enabled; }

private:
    std::vector<std::string> m_Lines;
    bool                     m_Enabled{true};
};

} // namespace NF::Game
