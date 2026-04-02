#include "Game/Voxel/ChunkMap.h"

namespace NF::Game {

Chunk* ChunkMap::CreateChunk(const ChunkCoord& coord)
{
    auto it = m_Chunks.find(coord);
    if (it != m_Chunks.end()) {
        // Replace existing chunk.
        it->second = std::make_unique<Chunk>(coord);
        return it->second.get();
    }
    auto [ins, ok] = m_Chunks.emplace(coord, std::make_unique<Chunk>(coord));
    (void)ok;
    return ins->second.get();
}

Chunk* ChunkMap::GetChunk(const ChunkCoord& coord) noexcept
{
    auto it = m_Chunks.find(coord);
    return (it != m_Chunks.end()) ? it->second.get() : nullptr;
}

const Chunk* ChunkMap::GetChunk(const ChunkCoord& coord) const noexcept
{
    auto it = m_Chunks.find(coord);
    return (it != m_Chunks.end()) ? it->second.get() : nullptr;
}

Chunk* ChunkMap::GetOrCreateChunk(const ChunkCoord& coord)
{
    auto it = m_Chunks.find(coord);
    if (it != m_Chunks.end()) return it->second.get();
    auto [ins, ok] = m_Chunks.emplace(coord, std::make_unique<Chunk>(coord));
    (void)ok;
    return ins->second.get();
}

void ChunkMap::UnloadChunk(const ChunkCoord& coord)
{
    m_Chunks.erase(coord);
}

void ChunkMap::Clear() noexcept
{
    m_Chunks.clear();
}

bool ChunkMap::HasChunk(const ChunkCoord& coord) const noexcept
{
    return m_Chunks.count(coord) > 0;
}

std::vector<Chunk*> ChunkMap::GetDirtyChunks()
{
    std::vector<Chunk*> result;
    for (auto& [coord, chunk] : m_Chunks) {
        if (chunk->IsDirty())
            result.push_back(chunk.get());
    }
    return result;
}

std::vector<ChunkCoord> ChunkMap::GetLoadedCoords() const
{
    std::vector<ChunkCoord> coords;
    coords.reserve(m_Chunks.size());
    for (const auto& [coord, chunk] : m_Chunks)
        coords.push_back(coord);
    return coords;
}

} // namespace NF::Game
