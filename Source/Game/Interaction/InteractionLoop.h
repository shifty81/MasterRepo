#pragma once
#include "Game/Interaction/RigState.h"
#include "Game/Interaction/Inventory.h"
#include "Game/Interaction/MiningTool.h"
#include "Game/Voxel/VoxelEditApi.h"

namespace NF::Game {

/// @brief Result of a placement action.
enum class PlaceResult {
    Success,    ///< Voxel placed and item deducted from inventory.
    NoItem,     ///< Inventory does not have the required resource.
    OutOfBounds ///< World position was invalid.
};

/// @brief Result of a repair action.
enum class RepairResult {
    Success,    ///< Voxel repaired and item deducted from inventory.
    NoItem,     ///< Inventory does not have the required resource.
    OutOfBounds ///< World position was invalid.
};

/// @brief Coordinates the Phase 3 first interaction loop.
///
/// Loop shape: spawn → mine (collect resources) → place / repair → repeat.
///
/// The InteractionLoop owns the player rig and inventory; it borrows a
/// VoxelEditApi from GameWorld.  Call Init() with the api pointer before
/// using Mine(), Place(), or Repair().
class InteractionLoop {
public:
    InteractionLoop() = default;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// @brief Provide the VoxelEditApi to use for world modifications.
    ///        Must be called before Mine / Place / Repair.
    void Init(VoxelEditApi* api) noexcept { m_Api = api; }

    /// @brief Per-frame update: ticks the rig (energy recharge, etc.).
    void Tick(float dt) noexcept;

    /// @brief Reset rig and inventory to initial state.
    void Reset() noexcept;

    // -------------------------------------------------------------------------
    // Actions
    // -------------------------------------------------------------------------

    /// @brief Mine the voxel at (wx, wy, wz).
    ///
    /// On success the extracted resource is automatically deposited in the
    /// inventory.  Returns the raw MiningResult so callers can inspect it.
    MiningResult Mine(int32_t wx, int32_t wy, int32_t wz);

    /// @brief Place a resource from inventory at (wx, wy, wz).
    ///
    /// Consumes one item of @p type from the inventory on success.
    PlaceResult Place(int32_t wx, int32_t wy, int32_t wz, ResourceType type);

    /// @brief Repair the voxel at (wx, wy, wz) using a resource from inventory.
    ///
    /// Restores the target voxel to @p repairTo and deducts one item of
    /// @p repairTo's resource type from the inventory.
    RepairResult Repair(int32_t wx, int32_t wy, int32_t wz, ResourceType type);

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    [[nodiscard]] RigState&       GetRig()             noexcept { return m_Rig; }
    [[nodiscard]] const RigState& GetRig()       const noexcept { return m_Rig; }
    [[nodiscard]] Inventory&      GetInventory()       noexcept { return m_Inventory; }
    [[nodiscard]] const Inventory& GetInventory() const noexcept { return m_Inventory; }

private:
    RigState    m_Rig;
    Inventory   m_Inventory;
    MiningTool  m_MiningTool;
    VoxelEditApi* m_Api{nullptr};
};

} // namespace NF::Game
