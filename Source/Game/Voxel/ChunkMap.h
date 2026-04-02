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
