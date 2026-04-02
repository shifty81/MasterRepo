#pragma once
#include "Game/Voxel/VoxelType.h"
#include <cstdint>

namespace NF::Game {

/// @brief Resource types that can be gathered, carried, and placed.
///
/// IDs are intentionally aligned with VoxelType so that conversion is trivial.
enum class ResourceType : uint8_t {
    None    = 0,
    Stone   = 1,
    Ore     = 2,
    Dirt    = 3,
    Rock    = 4,
    Metal   = 5,
    Ice     = 6,
    Organic = 7,
};

/// @brief A stack of identical resource items.
struct ResourceStack {
    ResourceType type{ResourceType::None};
    uint32_t     count{0};

    /// @brief Returns true when the stack carries no items.
    [[nodiscard]] bool IsEmpty() const noexcept {
        return count == 0 || type == ResourceType::None;
    }
};

/// @brief Convert a VoxelId to its corresponding ResourceType.
///
/// ResourceType IDs mirror VoxelType IDs for ids 1..7; unknown ids return None.
[[nodiscard]] inline ResourceType VoxelTypeToResource(VoxelId id) noexcept {
    if (id >= 1 && id <= 7) return static_cast<ResourceType>(id);
    return ResourceType::None;
}

/// @brief Convert a ResourceType back to its VoxelId for placement.
[[nodiscard]] inline VoxelId ResourceToVoxelId(ResourceType r) noexcept {
    return static_cast<VoxelId>(r);
}

/// @brief Human-readable name for a resource type.
[[nodiscard]] inline const char* ResourceTypeName(ResourceType r) noexcept {
    switch (r) {
        case ResourceType::Stone:   return "Stone";
        case ResourceType::Ore:     return "Ore";
        case ResourceType::Dirt:    return "Dirt";
        case ResourceType::Rock:    return "Rock";
        case ResourceType::Metal:   return "Metal";
        case ResourceType::Ice:     return "Ice";
        case ResourceType::Organic: return "Organic";
        default:                    return "None";
    }
}

} // namespace NF::Game
