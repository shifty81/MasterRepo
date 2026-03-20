// terrain_prototype.cpp — Standalone prototype, not linked to engine
// Tests: chunk streaming, LOD transitions, noise-based height
#include <iostream>
#include <vector>
#include <cmath>
#include <unordered_map>
#include <cstdint>
#include <string>
#include <functional>

namespace Experiments {

// ──────────────────────────────────────────────────────────────
// ChunkKey — integer chunk coordinate pair used as map key
// ──────────────────────────────────────────────────────────────

struct ChunkKey {
    int cx = 0;
    int cz = 0;
    bool operator==(const ChunkKey& o) const { return cx == o.cx && cz == o.cz; }
};

struct ChunkKeyHash {
    size_t operator()(const ChunkKey& k) const noexcept {
        return std::hash<int64_t>{}(
            (static_cast<int64_t>(k.cx) << 32) | static_cast<uint32_t>(k.cz));
    }
};

// ──────────────────────────────────────────────────────────────
// TerrainChunk — 16×16 height grid with LOD level
// ──────────────────────────────────────────────────────────────

struct TerrainChunk {
    float heightData[16][16] = {};
    int   lod    = 0;
    bool  loaded = false;
};

// ──────────────────────────────────────────────────────────────
// ChunkManager — streaming chunk system
// ──────────────────────────────────────────────────────────────

class ChunkManager {
public:
    // Generate height data for a chunk using sin/cos noise
    void GenerateChunk(int cx, int cz) {
        TerrainChunk chunk;
        chunk.lod    = ComputeLOD(cx, cz);
        chunk.loaded = true;
        for (int z = 0; z < 16; ++z) {
            for (int x = 0; x < 16; ++x) {
                float wx = static_cast<float>(cx * 16 + x);
                float wz = static_cast<float>(cz * 16 + z);
                chunk.heightData[z][x] =
                    20.0f * std::sin(wx * 0.05f) * std::cos(wz * 0.05f)
                  + 10.0f * std::sin(wx * 0.13f + wz * 0.07f)
                  +  5.0f * std::cos(wx * 0.23f - wz * 0.17f);
            }
        }
        m_chunks[{cx, cz}] = chunk;
    }

    // Load all chunks within Manhattan-distance radius around (cx, cz)
    void LoadAround(int cx, int cz, int radius) {
        for (int dz = -radius; dz <= radius; ++dz) {
            for (int dx = -radius; dx <= radius; ++dx) {
                ChunkKey key{ cx + dx, cz + dz };
                if (m_chunks.find(key) == m_chunks.end()) {
                    GenerateChunk(key.cx, key.cz);
                }
            }
        }
    }

    // Unload chunks further than `radius` from (cx, cz)
    void UnloadFar(int cx, int cz, int radius) {
        auto it = m_chunks.begin();
        while (it != m_chunks.end()) {
            int dx = std::abs(it->first.cx - cx);
            int dz = std::abs(it->first.cz - cz);
            if (dx > radius || dz > radius)
                it = m_chunks.erase(it);
            else
                ++it;
        }
    }

    size_t GetChunkCount() const { return m_chunks.size(); }

    const TerrainChunk* GetChunk(int cx, int cz) const {
        auto it = m_chunks.find({cx, cz});
        if (it == m_chunks.end()) return nullptr;
        return &it->second;
    }

private:
    // Chunks farther from origin get lower LOD (coarser)
    static int ComputeLOD(int cx, int cz) {
        int dist = std::abs(cx) + std::abs(cz);
        if (dist <= 2)  return 0;
        if (dist <= 5)  return 1;
        if (dist <= 10) return 2;
        return 3;
    }

    std::unordered_map<ChunkKey, TerrainChunk, ChunkKeyHash> m_chunks;
};

} // namespace Experiments

// ──────────────────────────────────────────────────────────────
// Standalone main
// ──────────────────────────────────────────────────────────────

int main() {
    using namespace Experiments;

    std::cout << "[terrain_prototype] Starting...\n";

    ChunkManager mgr;

    // Load a 7×7 grid around origin
    mgr.LoadAround(0, 0, 3);
    std::cout << "[terrain_prototype] Chunks loaded: " << mgr.GetChunkCount() << "\n";

    // Print a sample height from the centre chunk
    const TerrainChunk* centre = mgr.GetChunk(0, 0);
    if (centre) {
        std::cout << "[terrain_prototype] Centre chunk (0,0) LOD=" << centre->lod
                  << "  height[8][8]=" << centre->heightData[8][8] << "\n";
    }

    // Simulate player moving to (5, 5) — unload old chunks beyond radius 2
    mgr.LoadAround(5, 5, 3);
    std::cout << "[terrain_prototype] After LoadAround(5,5,3): "
              << mgr.GetChunkCount() << " chunks\n";

    mgr.UnloadFar(5, 5, 3);
    std::cout << "[terrain_prototype] After UnloadFar(5,5,3): "
              << mgr.GetChunkCount() << " chunks\n";

    // Print LOD distribution
    for (int dz = -3; dz <= 3; ++dz) {
        for (int dx = -3; dx <= 3; ++dx) {
            const TerrainChunk* c = mgr.GetChunk(5 + dx, 5 + dz);
            if (c) std::cout << c->lod;
            else   std::cout << '.';
        }
        std::cout << "\n";
    }

    std::cout << "[terrain_prototype] Done.\n";
    return 0;
}
