#include "Engine/Terrain/TerrainSystem.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Engine {

// ── LODPolicy ─────────────────────────────────────────────────────────────
int32_t LODPolicy::SelectLOD(float distance) const {
    for (size_t i = 0; i < distanceThresholds.size(); ++i)
        if (distance <= distanceThresholds[i])
            return static_cast<int32_t>(i);
    return static_cast<int32_t>(maxLOD);
}

// ── TerrainChunk ─────────────────────────────────────────────────────────
float TerrainChunk::SampleHeight(float localX, float localZ) const {
    if (heights.empty()) return 0.0f;
    // Bilinear interpolation
    float cx = localX / cellSize;
    float cz = localZ / cellSize;
    int32_t ix = static_cast<int32_t>(cx);
    int32_t iz = static_cast<int32_t>(cz);
    int32_t maxI = static_cast<int32_t>(resolution) - 1;
    ix = std::max(0, std::min(ix, maxI - 1));
    iz = std::max(0, std::min(iz, maxI - 1));
    float fx = cx - ix;
    float fz = cz - iz;
    auto idx = [&](int32_t r, int32_t c) -> float {
        r = std::max(0, std::min(r, maxI));
        c = std::max(0, std::min(c, maxI));
        return heights[static_cast<size_t>(r) * resolution + static_cast<size_t>(c)];
    };
    float h00 = idx(iz,   ix);
    float h10 = idx(iz,   ix+1);
    float h01 = idx(iz+1, ix);
    float h11 = idx(iz+1, ix+1);
    return (h00*(1-fx)*(1-fz) + h10*fx*(1-fz) +
            h01*(1-fx)*fz     + h11*fx*fz) * heightScale;
}

Math::Vec3 TerrainChunk::SampleNormal(float localX, float localZ) const {
    if (normals.empty()) return {0.0f, 1.0f, 0.0f};
    float cx = localX / cellSize;
    float cz = localZ / cellSize;
    int32_t ix = static_cast<int32_t>(cx);
    int32_t iz = static_cast<int32_t>(cz);
    int32_t maxI = static_cast<int32_t>(resolution) - 1;
    ix = std::max(0, std::min(ix, maxI));
    iz = std::max(0, std::min(iz, maxI));
    return normals[static_cast<size_t>(iz) * resolution + static_cast<size_t>(ix)];
}

void TerrainChunk::ComputeNormals() {
    normals.resize(static_cast<size_t>(resolution) * resolution, {0.0f, 1.0f, 0.0f});
    auto h = [&](int32_t r, int32_t c) -> float {
        int32_t maxI = static_cast<int32_t>(resolution) - 1;
        r = std::max(0, std::min(r, maxI));
        c = std::max(0, std::min(c, maxI));
        return heights[static_cast<size_t>(r) * resolution + static_cast<size_t>(c)] * heightScale;
    };
    for (uint32_t r = 0; r < resolution; ++r) {
        for (uint32_t c = 0; c < resolution; ++c) {
            float hL = h(r, static_cast<int32_t>(c)-1);
            float hR = h(r, static_cast<int32_t>(c)+1);
            float hD = h(static_cast<int32_t>(r)-1, c);
            float hU = h(static_cast<int32_t>(r)+1, c);
            Math::Vec3 n{ hL-hR, 2.0f*cellSize, hD-hU };
            float len = std::sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
            if (len > 0.0f) { n.x/=len; n.y/=len; n.z/=len; }
            normals[static_cast<size_t>(r) * resolution + c] = n;
        }
    }
}

// ── Impl ─────────────────────────────────────────────────────────────────
struct TerrainSystem::Impl {
    std::unordered_map<ChunkCoord, TerrainChunk, ChunkCoordHash> chunks;
    uint32_t chunkRes{65};
    float    cellSize{1.0f};
    float    heightScale{64.0f};
    int32_t  streamRadius{3};
    LODPolicy lodPolicy;
    HeightmapSourceCb heightSource;
};

TerrainSystem::TerrainSystem() : m_impl(new Impl()) {
    m_impl->lodPolicy.distanceThresholds = {64.0f, 128.0f, 256.0f, 512.0f};
    m_impl->lodPolicy.maxLOD = 4;
}
TerrainSystem::~TerrainSystem() { delete m_impl; }

void TerrainSystem::SetChunkResolution(uint32_t res) { m_impl->chunkRes = res; }
void TerrainSystem::SetCellSize(float s) { m_impl->cellSize = s; }
void TerrainSystem::SetHeightScale(float s) { m_impl->heightScale = s; }
void TerrainSystem::SetStreamRadius(int32_t r) { m_impl->streamRadius = r; }
void TerrainSystem::SetHeightSource(HeightmapSourceCb cb) { m_impl->heightSource = std::move(cb); }
void TerrainSystem::SetLODPolicy(const LODPolicy& p) { m_impl->lodPolicy = p; }

ChunkCoord TerrainSystem::WorldToChunk(float wx, float wz) const {
    float chunkWorld = (m_impl->chunkRes - 1) * m_impl->cellSize;
    return {static_cast<int32_t>(std::floor(wx / chunkWorld)),
            static_cast<int32_t>(std::floor(wz / chunkWorld))};
}

TerrainChunk* TerrainSystem::loadChunk(ChunkCoord coord) {
    auto& chunk = m_impl->chunks[coord];
    chunk.coord       = coord;
    chunk.resolution  = m_impl->chunkRes;
    chunk.cellSize    = m_impl->cellSize;
    chunk.heightScale = m_impl->heightScale;
    chunk.heights.assign(static_cast<size_t>(m_impl->chunkRes) * m_impl->chunkRes, 0.0f);
    if (m_impl->heightSource) m_impl->heightSource(chunk);
    chunk.ComputeNormals();
    chunk.loaded = true;
    return &chunk;
}

void TerrainSystem::unloadChunk(ChunkCoord coord) {
    m_impl->chunks.erase(coord);
}

void TerrainSystem::UpdateStreaming(float viewX, float viewZ) {
    ChunkCoord centre = WorldToChunk(viewX, viewZ);
    int32_t r = m_impl->streamRadius;

    // Load needed chunks
    for (int32_t dz = -r; dz <= r; ++dz) {
        for (int32_t dx = -r; dx <= r; ++dx) {
            ChunkCoord c{centre.cx + dx, centre.cz + dz};
            if (!m_impl->chunks.count(c)) loadChunk(c);
        }
    }
    // Unload distant chunks
    std::vector<ChunkCoord> toRemove;
    for (auto& [c, _] : m_impl->chunks) {
        if (std::abs(c.cx - centre.cx) > r + 1 ||
            std::abs(c.cz - centre.cz) > r + 1)
            toRemove.push_back(c);
    }
    for (auto& c : toRemove) unloadChunk(c);
}

size_t TerrainSystem::LoadedChunkCount() const { return m_impl->chunks.size(); }

const TerrainChunk* TerrainSystem::GetChunk(ChunkCoord coord) const {
    auto it = m_impl->chunks.find(coord);
    return it != m_impl->chunks.end() ? &it->second : nullptr;
}
TerrainChunk* TerrainSystem::GetChunk(ChunkCoord coord) {
    auto it = m_impl->chunks.find(coord);
    return it != m_impl->chunks.end() ? &it->second : nullptr;
}

std::vector<ChunkCoord> TerrainSystem::LoadedChunks() const {
    std::vector<ChunkCoord> out;
    for (const auto& [c, _] : m_impl->chunks) out.push_back(c);
    return out;
}

float TerrainSystem::SampleHeight(float wx, float wz) const {
    ChunkCoord cc = WorldToChunk(wx, wz);
    const auto* chunk = GetChunk(cc);
    if (!chunk || !chunk->loaded) return 0.0f;
    float lx = wx - chunk->WorldX();
    float lz = wz - chunk->WorldZ();
    return chunk->SampleHeight(lx, lz);
}

Math::Vec3 TerrainSystem::SampleNormal(float wx, float wz) const {
    ChunkCoord cc = WorldToChunk(wx, wz);
    const auto* chunk = GetChunk(cc);
    if (!chunk || !chunk->loaded) return {0.0f, 1.0f, 0.0f};
    float lx = wx - chunk->WorldX();
    float lz = wz - chunk->WorldZ();
    return chunk->SampleNormal(lx, lz);
}

TerrainHit TerrainSystem::RayCast(const Math::Vec3& origin,
                                   const Math::Vec3& direction,
                                   float maxDistance,
                                   float stepSize) const
{
    float len = std::sqrt(direction.x*direction.x + direction.y*direction.y + direction.z*direction.z);
    if (len < 1e-6f) return {};
    Math::Vec3 dir{direction.x/len, direction.y/len, direction.z/len};
    float dist = 0.0f;
    float prevH = origin.y - SampleHeight(origin.x, origin.z);
    while (dist < maxDistance) {
        float x = origin.x + dir.x * dist;
        float y = origin.y + dir.y * dist;
        float z = origin.z + dir.z * dist;
        float h = SampleHeight(x, z);
        float diff = y - h;
        if (diff <= 0.0f) {
            // Linear interpolation to refine
            float t = prevH / (prevH - diff);
            float rx = origin.x + dir.x * (dist - stepSize + t * stepSize);
            float rz = origin.z + dir.z * (dist - stepSize + t * stepSize);
            float ry = SampleHeight(rx, rz);
            TerrainHit hit;
            hit.hit      = true;
            hit.point    = {rx, ry, rz};
            hit.normal   = SampleNormal(rx, rz);
            hit.distance = dist - stepSize + t * stepSize;
            hit.chunk    = WorldToChunk(rx, rz);
            return hit;
        }
        prevH = diff;
        dist += stepSize;
    }
    return {};
}

void TerrainSystem::Sculpt(float wx, float wz, float radius, float delta) {
    // Apply delta to all heights within radius (paint brush)
    int32_t chunkRange = static_cast<int32_t>(std::ceil(radius / ((m_impl->chunkRes-1)*m_impl->cellSize))) + 1;
    ChunkCoord centre = WorldToChunk(wx, wz);
    for (int32_t dz = -chunkRange; dz <= chunkRange; ++dz) {
        for (int32_t dx = -chunkRange; dx <= chunkRange; ++dx) {
            auto* chunk = GetChunk({centre.cx+dx, centre.cz+dz});
            if (!chunk) continue;
            float ox = chunk->WorldX();
            float oz = chunk->WorldZ();
            for (uint32_t r = 0; r < chunk->resolution; ++r) {
                for (uint32_t c = 0; c < chunk->resolution; ++c) {
                    float vx = ox + c * chunk->cellSize;
                    float vz = oz + r * chunk->cellSize;
                    float dist = std::sqrt((vx-wx)*(vx-wx) + (vz-wz)*(vz-wz));
                    if (dist <= radius) {
                        float w = 1.0f - dist/radius;
                        chunk->heights[r * chunk->resolution + c] += delta * w;
                    }
                }
            }
            chunk->ComputeNormals();
        }
    }
}

} // namespace Engine
