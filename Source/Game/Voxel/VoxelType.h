#pragma once
#include <cstdint>
#include <string_view>

namespace NF::Game {

/// @brief Integer identifier for a voxel type.
///
/// ID 0 is always Air (empty).  All other IDs are registered types.
/// Stored as uint8_t inside a Chunk — maximum 255 registered types.
using VoxelId = uint8_t;

/// @brief Well-known built-in voxel type IDs.
enum class VoxelType : VoxelId {
    Air      = 0, ///< Empty / unoccupied voxel.
    Stone    = 1, ///< Basic structural material.
    Ore      = 2, ///< Minable resource deposit.
    Dirt     = 3, ///< Loose surface material.
    Rock     = 4, ///< Hard geological voxel.
    Metal    = 5, ///< Refined metal block.
    Ice      = 6, ///< Frozen volatile material.
    Organic  = 7, ///< Organic / biome material.
};

/// @brief Descriptor for a single voxel type.
struct VoxelTypeInfo {
    VoxelId     id{0};
    const char* name{"Unknown"};
    bool        isSolid{false};    ///< Solid voxels block movement and mining.
    bool        isMineable{false}; ///< Can be removed via mining operations.
    uint8_t     hardness{1};       ///< Mining cost multiplier (1 = base).
};

/// @brief Static registry of built-in voxel type info.
///
/// Returns info for well-known VoxelType values.
/// Returns an Air descriptor for any unrecognised ID.
[[nodiscard]] inline const VoxelTypeInfo& GetVoxelTypeInfo(VoxelId id) noexcept {
    static const VoxelTypeInfo kTypes[] = {
        { static_cast<VoxelId>(VoxelType::Air),     "Air",     false, false, 0 },
        { static_cast<VoxelId>(VoxelType::Stone),   "Stone",   true,  true,  2 },
        { static_cast<VoxelId>(VoxelType::Ore),     "Ore",     true,  true,  3 },
        { static_cast<VoxelId>(VoxelType::Dirt),    "Dirt",    true,  true,  1 },
        { static_cast<VoxelId>(VoxelType::Rock),    "Rock",    true,  true,  4 },
        { static_cast<VoxelId>(VoxelType::Metal),   "Metal",   true,  true,  5 },
        { static_cast<VoxelId>(VoxelType::Ice),     "Ice",     true,  true,  2 },
        { static_cast<VoxelId>(VoxelType::Organic), "Organic", true,  true,  1 },
    };
    constexpr VoxelId kKnownCount =
        static_cast<VoxelId>(sizeof(kTypes) / sizeof(kTypes[0]));
    if (id < kKnownCount) return kTypes[id];
    return kTypes[0]; // fallback to Air
}

/// @brief Convenience overload accepting the enum directly.
[[nodiscard]] inline const VoxelTypeInfo& GetVoxelTypeInfo(VoxelType type) noexcept {
    return GetVoxelTypeInfo(static_cast<VoxelId>(type));
}

} // namespace NF::Game
