#pragma once
#include "Game/Interaction/MiningTool.h"
#include "Game/Voxel/VoxelEditApi.h"
#include "Game/Interaction/RigState.h"

namespace NF::Game::Gameplay {

class ProgressionSystem;

/// @brief Mining-tool tier — higher tiers deal more damage and yield more resources.
enum class MiningTier : uint8_t {
    Tier1 = 0, ///< Default starter tier.
    Tier2 = 1, ///< Unlocked at progression level 3.
    Tier3 = 2, ///< Unlocked at progression level 7.
};

/// @brief Full mining-progression system.
///
/// Wraps the low-level MiningTool and augments it with:
///   - Tier-based damage / yield / energy multipliers.
///   - Per-swing XP awards to a linked ProgressionSystem.
///   - Automatic tier selection based on the player's current level.
class MiningSystem {
public:
    /// @brief XP awarded to the ProgressionSystem for each successfully mined voxel.
    static constexpr uint32_t kXpPerMine = 10;

    MiningSystem() = default;

    // -------------------------------------------------------------------------
    // Tier
    // -------------------------------------------------------------------------

    /// @brief Manually override the active tier.
    ///        Normally set automatically from the ProgressionSystem's level.
    void SetTier(MiningTier tier) noexcept { m_Tier = tier; }

    /// @brief Active tier, updated by @c SyncTier.
    [[nodiscard]] MiningTier GetTier() const noexcept { return m_Tier; }

    /// @brief Update the active tier based on the progression system's level.
    ///        Call after awarding XP so the player immediately benefits.
    void SyncTier(const ProgressionSystem& prog) noexcept;

    // -------------------------------------------------------------------------
    // Energy cost per tier
    // -------------------------------------------------------------------------
    [[nodiscard]] float GetEnergyCost() const noexcept;

    /// @brief Yield multiplier for the active tier (whole-number scale).
    [[nodiscard]] uint32_t GetYieldMultiplier() const noexcept;

    // -------------------------------------------------------------------------
    // Mining action
    // -------------------------------------------------------------------------

    /// @brief Attempt to mine the voxel at (wx, wy, wz).
    ///
    /// Awards XP to @p prog on a successful mine.
    /// @param prog  Optional pointer to the progression system (may be nullptr).
    NF::Game::MiningResult Mine(NF::Game::VoxelEditApi& api,
                                NF::Game::RigState&     rig,
                                int32_t wx, int32_t wy, int32_t wz,
                                ProgressionSystem*      prog = nullptr);

    // -------------------------------------------------------------------------
    // Stats
    // -------------------------------------------------------------------------

    /// @brief Total voxels successfully mined since creation / last Reset().
    [[nodiscard]] uint32_t GetTotalMined() const noexcept { return m_TotalMined; }

    /// @brief Reset per-session statistics (does not reset tier or progression).
    void ResetStats() noexcept { m_TotalMined = 0; }

private:
    MiningTier        m_Tier{MiningTier::Tier1};
    uint32_t          m_TotalMined{0};
    NF::Game::MiningTool m_Tool;
};

} // namespace NF::Game::Gameplay
