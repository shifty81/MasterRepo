#pragma once
#include "Renderer/Mesh/MeshRenderer.h"
#include "Game/Voxel/Chunk.h"
#include "Game/Voxel/ChunkCoord.h"

namespace NF::Game {

/// @brief Generates CPU-side @c MeshData from a voxel @c Chunk.
///
/// Uses culled-face meshing: only emits a quad face between a solid voxel
/// and an adjacent air (or out-of-bounds) neighbour.  Each face receives a
/// flat normal pointing outward and a per-voxel-type colour encoded in the
/// vertex @c TexCoord.X channel (palette index).
///
/// The mesh origin is at the chunk's world-space origin so that the model
/// transform for every chunk is simply @c Matrix4x4::Identity().
class VoxelMesher {
public:
    /// @brief Generate the triangle mesh for a single chunk.
    ///
    /// @param chunk        The chunk to mesh.
    /// @param neighbours   Six optional chunk pointers ordered
    ///                     -X, +X, -Y, +Y, -Z, +Z.
    ///                     If a pointer is null, faces on that boundary are
    ///                     always emitted (open-air boundary).
    /// @return CPU-side vertex and index data ready for @c Mesh::Upload().
    [[nodiscard]] MeshData Generate(const Chunk& chunk,
                                    const Chunk* neighbours[6]) const;

    /// @brief Convenience overload that assumes all neighbours are open air.
    [[nodiscard]] MeshData Generate(const Chunk& chunk) const;

    /// @brief Return the colour associated with a @c VoxelId.
    ///
    /// Used to encode per-face colour so the shader can render each voxel
    /// type with a distinct appearance.
    [[nodiscard]] static Vector3 VoxelColor(VoxelId id) noexcept;
};

} // namespace NF::Game
