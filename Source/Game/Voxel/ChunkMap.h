#pragma once
#include "Game/Voxel/Chunk.h"
#include "Game/Voxel/ChunkCoord.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace NF::Game {

/// @brief Manages the live set of chunks in the world.
///
/// Chunks are owned by the map as unique_ptr and keyed by their
/// @c ChunkCoord.  Callers may create, retrieve, and release chunks;
/// the map reports which chunks are dirty for serialization.
class ChunkMap {
public:
    ChunkMap() = default;

    // ---- Chunk lifecycle ----------------------------------------------------

    /// @brief Create or replace the chunk at @p coord.
    /// @return Non-owning pointer to the new chunk (valid while the map lives).
    Chunk* CreateChunk(const ChunkCoord& coord);

    /// @brief Retrieve an existing chunk, or nullptr if none.
    [[nodiscard]] Chunk* GetChunk(const ChunkCoord& coord) noexcept;
    [[nodiscard]] const Chunk* GetChunk(const ChunkCoord& coord) const noexcept;

    /// @brief Get the chunk at @p coord, creating it if absent.
    Chunk* GetOrCreateChunk(const ChunkCoord& coord);

    /// @brief Remove the chunk at @p coord (if it exists).
    void UnloadChunk(const ChunkCoord& coord);

    /// @brief Remove all chunks.
    void Clear() noexcept;

    // ---- Queries ------------------------------------------------------------

    [[nodiscard]] bool HasChunk(const ChunkCoord& coord) const noexcept;

    /// @brief Return the number of loaded chunks.
    [[nodiscard]] size_t ChunkCount() const noexcept { return m_Chunks.size(); }

    /// @brief Collect pointers to all dirty chunks (does not clear dirty flags).
    [[nodiscard]] std::vector<Chunk*> GetDirtyChunks();

    /// @brief Collect all loaded chunk coords.
    [[nodiscard]] std::vector<ChunkCoord> GetLoadedCoords() const;

    // ---- Collision / spatial queries ----------------------------------------

    /// @brief Return true if the voxel at world position (wx,wy,wz) is solid.
    ///
    /// If the chunk containing this voxel is not loaded, returns @p unloadedSolid
    /// (default: true — unloaded voxels block movement).
    [[nodiscard]] bool IsSolidAt(int32_t wx, int32_t wy, int32_t wz,
                                 bool unloadedSolid = true) const noexcept;

    /// @brief Result of a voxel raycast query.
    struct VoxelHit {
        int32_t x{0}, y{0}, z{0};             ///< World position of the hit voxel.
        int32_t prevX{0}, prevY{0}, prevZ{0};  ///< Position of the air voxel before the hit.
        VoxelId id{0};                          ///< Type of the hit voxel.
        float   distance{0.f};                  ///< Distance from origin to hit.
        bool    hit{false};                     ///< True if a solid voxel was found.
    };

    /// @brief Cast a ray through the voxel grid using DDA (3-D).
    ///
    /// Walks the ray voxel-by-voxel and returns the first solid voxel hit.
    /// @param ox,oy,oz  Ray origin in world-space floats.
    /// @param dx,dy,dz  Ray direction (need not be normalised).
    /// @param maxDist   Maximum distance to traverse.
    /// @return A @c VoxelHit with @c hit==true on intersection.
    [[nodiscard]] VoxelHit RaycastVoxel(float ox, float oy, float oz,
                                        float dx, float dy, float dz,
                                        float maxDist = 64.f) const noexcept;

    // ---- Iteration ----------------------------------------------------------

    /// @brief Invoke @p fn for every loaded chunk.
    template<typename Fn>
    void ForEach(Fn&& fn) {
        for (auto& [coord, chunk] : m_Chunks)
            fn(*chunk);
    }

private:
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>> m_Chunks;
};

} // namespace NF::Game
