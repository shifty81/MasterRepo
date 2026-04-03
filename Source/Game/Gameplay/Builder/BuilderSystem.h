#pragma once
#include "Game/Interaction/ResourceItem.h"
#include "Game/Interaction/Inventory.h"
#include "Game/Voxel/VoxelEditApi.h"
#include <cstdint>

namespace NF::Game::Gameplay {

class ProgressionSystem;

/// @brief Result of a Build action.
enum class BuildResult : uint8_t {
    Success,         ///< Voxel placed and resources deducted.
    InsufficientResources, ///< Not enough resources in inventory.
    OutOfBounds,     ///< Target position was outside the world.
    InvalidRecipe,   ///< No recipe exists for the requested type.
    LockedTier,      ///< Requires the AdvancedBuild skill unlock.
};

/// @brief Result of a Teardown action.
enum class TeardownResult : uint8_t {
    Success,      ///< Voxel removed and partial resources returned.
    EmptyCell,    ///< No solid voxel was present.
    OutOfBounds,  ///< Target position was outside the world.
};

/// @brief Per-resource placement recipe.
struct BuildRecipe {
    NF::Game::ResourceType resource{NF::Game::ResourceType::None};
    uint32_t               cost{1};     ///< Resources consumed to place 1 voxel.
    uint32_t               refund{0};   ///< Resources returned on teardown.
    bool                   requiresAdvancedBuild{false};
};

/// @brief Voxel-based construction system.
///
/// Provides recipe-based placement and teardown, with resource deduction from
/// the player's Inventory and optional XP awards through ProgressionSystem.
class BuilderSystem {
public:
    /// @brief XP awarded per successful voxel placement.
    static constexpr uint32_t kXpPerBuild = 5;

    BuilderSystem();

    // -------------------------------------------------------------------------
    // Recipes
    // -------------------------------------------------------------------------

    /// @brief Return the recipe for @p type, or nullptr if none exists.
    [[nodiscard]] const BuildRecipe* FindRecipe(NF::Game::ResourceType type) const noexcept;

    // -------------------------------------------------------------------------
    // Actions
    // -------------------------------------------------------------------------

    /// @brief Place a voxel of @p type at (wx, wy, wz).
    ///
    /// Deducts recipe cost from @p inv.  Optionally awards XP to @p prog.
    BuildResult Build(NF::Game::VoxelEditApi& api,
                      NF::Game::Inventory&    inv,
                      int32_t wx, int32_t wy, int32_t wz,
                      NF::Game::ResourceType type,
                      ProgressionSystem*     prog = nullptr);

    /// @brief Remove the voxel at (wx, wy, wz) and refund partial resources.
    TeardownResult Teardown(NF::Game::VoxelEditApi& api,
                            NF::Game::Inventory&    inv,
                            int32_t wx, int32_t wy, int32_t wz);

    // -------------------------------------------------------------------------
    // Stats
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t GetTotalBuilt()    const noexcept { return m_TotalBuilt; }
    [[nodiscard]] uint32_t GetTotalTornDown() const noexcept { return m_TotalTornDown; }

    void ResetStats() noexcept { m_TotalBuilt = 0; m_TotalTornDown = 0; }

private:
    static constexpr int kRecipeCount = 7;
    BuildRecipe  m_Recipes[kRecipeCount];
    uint32_t     m_TotalBuilt{0};
    uint32_t     m_TotalTornDown{0};
};

} // namespace NF::Game::Gameplay
