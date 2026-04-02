#include "Game/Voxel/ChunkMap.h"
#include <cmath>
#include <algorithm>

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

// ---------------------------------------------------------------------------
// Collision / spatial queries
// ---------------------------------------------------------------------------

bool ChunkMap::IsSolidAt(int32_t wx, int32_t wy, int32_t wz,
                         bool unloadedSolid) const noexcept
{
    const ChunkCoord cc = WorldToChunk(wx, wy, wz);
    const Chunk* chunk = GetChunk(cc);
    if (!chunk) return unloadedSolid;

    const LocalVoxelCoord lv = WorldToLocal(wx, wy, wz);
    const VoxelId id = chunk->GetVoxel(lv.X, lv.Y, lv.Z);
    return GetVoxelTypeInfo(id).isSolid;
}

ChunkMap::VoxelHit ChunkMap::RaycastVoxel(float ox, float oy, float oz,
                                           float dx, float dy, float dz,
                                           float maxDist) const noexcept
{
    VoxelHit result{};

    // Normalise direction.
    const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 1e-8f) return result;
    dx /= len;  dy /= len;  dz /= len;

    // DDA (Amanatides & Woo).
    int32_t ix = static_cast<int32_t>(std::floor(ox));
    int32_t iy = static_cast<int32_t>(std::floor(oy));
    int32_t iz = static_cast<int32_t>(std::floor(oz));

    const int32_t stepX = (dx >= 0.f) ? 1 : -1;
    const int32_t stepY = (dy >= 0.f) ? 1 : -1;
    const int32_t stepZ = (dz >= 0.f) ? 1 : -1;

    const float tDeltaX = (std::abs(dx) > 1e-8f) ? std::abs(1.f / dx) : 1e30f;
    const float tDeltaY = (std::abs(dy) > 1e-8f) ? std::abs(1.f / dy) : 1e30f;
    const float tDeltaZ = (std::abs(dz) > 1e-8f) ? std::abs(1.f / dz) : 1e30f;

    float tMaxX = (std::abs(dx) > 1e-8f)
        ? ((dx >= 0.f ? (ix + 1.f - ox) : (ox - ix)) * tDeltaX) : 1e30f;
    float tMaxY = (std::abs(dy) > 1e-8f)
        ? ((dy >= 0.f ? (iy + 1.f - oy) : (oy - iy)) * tDeltaY) : 1e30f;
    float tMaxZ = (std::abs(dz) > 1e-8f)
        ? ((dz >= 0.f ? (iz + 1.f - oz) : (oz - iz)) * tDeltaZ) : 1e30f;

    float t = 0.f;
    int32_t prevX = ix, prevY = iy, prevZ = iz;

    while (t < maxDist) {
        // Test current voxel.
        if (IsSolidAt(ix, iy, iz, false)) {
            result.x = ix;  result.y = iy;  result.z = iz;
            result.prevX = prevX;  result.prevY = prevY;  result.prevZ = prevZ;
            const ChunkCoord cc = WorldToChunk(ix, iy, iz);
            const Chunk* chunk = GetChunk(cc);
            if (chunk) {
                const LocalVoxelCoord lv = WorldToLocal(ix, iy, iz);
                result.id = chunk->GetVoxel(lv.X, lv.Y, lv.Z);
            }
            result.distance = t;
            result.hit = true;
            return result;
        }

        prevX = ix;  prevY = iy;  prevZ = iz;

        // Step to the next voxel boundary.
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                t = tMaxX;
                ix += stepX;
                tMaxX += tDeltaX;
            } else {
                t = tMaxZ;
                iz += stepZ;
                tMaxZ += tDeltaZ;
            }
        } else {
            if (tMaxY < tMaxZ) {
                t = tMaxY;
                iy += stepY;
                tMaxY += tDeltaY;
            } else {
                t = tMaxZ;
                iz += stepZ;
                tMaxZ += tDeltaZ;
            }
        }
    }

    return result;
}

} // namespace NF::Game
