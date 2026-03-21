#include "PCG/World/WorldChunkGen/WorldChunkGen.h"
#include <cmath>
#include <cstring>
#include <cstdint>
#include <unordered_map>
#include <list>
#include <algorithm>
#include <random>

namespace PCG {

// ── Simple hash for ChunkCoord ────────────────────────────────────────────
struct ChunkCoordHash {
    size_t operator()(const ChunkCoord& c) const noexcept {
        size_t h = std::hash<int32_t>()(c.cx);
        h ^= std::hash<int32_t>()(c.cz) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

// ── Value noise helpers ───────────────────────────────────────────────────
static float hash2(int32_t x, int32_t z, uint64_t seed) {
    uint64_t h = static_cast<uint64_t>(x) * 0xd2a98b26625eee7bULL ^
                 static_cast<uint64_t>(z) * 0xa37bbe8f0a256c17ULL ^ seed;
    h ^= (h >> 33); h *= 0xff51afd7ed558ccdULL;
    h ^= (h >> 33); h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= (h >> 33);
    return static_cast<float>(h & 0xFFFFFFFFULL) / static_cast<float>(0xFFFFFFFFULL);
}

static float smoothstep(float t) { return t * t * (3.0f - 2.0f * t); }

static float lerp(float a, float b, float t) { return a + (b - a) * t; }

static float valueNoise(float wx, float wz, uint64_t seed) {
    int32_t ix = static_cast<int32_t>(std::floor(wx));
    int32_t iz = static_cast<int32_t>(std::floor(wz));
    float fx = wx - static_cast<float>(ix);
    float fz = wz - static_cast<float>(iz);
    float sx = smoothstep(fx), sz = smoothstep(fz);
    float v00 = hash2(ix,   iz,   seed);
    float v10 = hash2(ix+1, iz,   seed);
    float v01 = hash2(ix,   iz+1, seed);
    float v11 = hash2(ix+1, iz+1, seed);
    return lerp(lerp(v00, v10, sx), lerp(v01, v11, sx), sz);
}

static float fbm(float wx, float wz, uint32_t octaves,
                 float lacunarity, float persistence, uint64_t seed) {
    float value = 0.0f, amplitude = 1.0f, frequency = 1.0f, maxVal = 0.0f;
    for (uint32_t i = 0; i < octaves; ++i) {
        value    += valueNoise(wx * frequency, wz * frequency, seed + i) * amplitude;
        maxVal   += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    return maxVal > 0.0f ? value / maxVal : 0.0f;
}

static const char* biomeForHeight(float h, float moisture, float temperature) {
    if (h < 0.3f) return "water";
    if (h > 0.85f) return "snow";
    if (temperature < 0.2f) return "tundra";
    if (moisture > 0.7f && temperature > 0.5f) return "jungle";
    if (moisture < 0.2f) return "desert";
    if (h > 0.65f) return "mountain";
    if (moisture > 0.4f) return "forest";
    return "grassland";
}

static uint8_t biomeId(const char* biome) {
    static const char* biomes[] = {
        "water","snow","tundra","jungle","desert","mountain","forest","grassland"
    };
    for (uint8_t i = 0; i < 8; ++i)
        if (std::strcmp(biomes[i], biome) == 0) return i;
    return 7;
}

// ── LRU cache entry ───────────────────────────────────────────────────────
using CacheKey  = ChunkCoord;
using CacheList = std::list<CacheKey>;

struct CacheEntry {
    WorldChunk      chunk;
    CacheList::iterator iter;
};

// ── Impl ─────────────────────────────────────────────────────────────────
struct WorldChunkGen::Impl {
    WorldChunkConfig cfg;
    std::unordered_map<ChunkCoord, CacheEntry, ChunkCoordHash> cache;
    CacheList                                                   lruList;
    std::vector<WorldChunkGen::ChunkReadyCb>                    callbacks;

    WorldChunk generate(int32_t cx, int32_t cz) {
        WorldChunk chunk;
        chunk.coord = {cx, cz};
        chunk.size  = cfg.chunkSize;
        chunk.samples.resize(static_cast<size_t>(cfg.chunkSize) * cfg.chunkSize);

        float invSize = 1.0f / static_cast<float>(cfg.chunkSize);
        float worldUnits = cfg.worldScale;

        for (uint32_t z = 0; z < cfg.chunkSize; ++z) {
            for (uint32_t x = 0; x < cfg.chunkSize; ++x) {
                float wx = (static_cast<float>(cx) + static_cast<float>(x) * invSize)
                           * worldUnits / cfg.heightScale;
                float wz = (static_cast<float>(cz) + static_cast<float>(z) * invSize)
                           * worldUnits / cfg.heightScale;

                float h = fbm(wx, wz, cfg.noiseOctaves,
                               cfg.noiseLacunarity, cfg.noisePersistence, cfg.seed);
                float moisture = fbm(wx + 100.5f, wz + 100.5f, 3,
                                      2.0f, 0.5f, cfg.seed + 1);
                float temperature = fbm(wx + 200.5f, wz + 200.5f, 3,
                                         2.0f, 0.5f, cfg.seed + 2);

                const char* biome = biomeForHeight(h, moisture, temperature);

                HeightSample& s   = chunk.samples[z * cfg.chunkSize + x];
                s.height          = h * cfg.heightScale;
                s.biomeId         = biomeId(biome);
                s.material        = biome;
                s.moisture        = moisture;
                s.temperature     = temperature;
            }
        }

        // Place decorations
        if (cfg.placeDecorations) {
            std::mt19937_64 rng(cfg.seed ^
                (static_cast<uint64_t>(cx) * 0x9e3779b97f4a7c15ULL) ^
                (static_cast<uint64_t>(cz) * 0x6c62272e07bb0142ULL));
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            for (uint32_t z = 0; z < cfg.chunkSize; ++z) {
                for (uint32_t x = 0; x < cfg.chunkSize; ++x) {
                    if (dist(rng) < cfg.decorationDensity) {
                        const HeightSample& s = chunk.samples[z * cfg.chunkSize + x];
                        if (s.height > cfg.seaLevel * cfg.heightScale) {
                            Decoration d;
                            d.localX    = x;
                            d.localZ    = z;
                            d.yOffset   = s.height;
                            d.scale     = 0.8f + dist(rng) * 0.4f;
                            d.rotation  = dist(rng) * 6.2831853f;
                            if (s.material == "forest" || s.material == "jungle")
                                d.type = dist(rng) > 0.3f ? "tree_pine" : "tree_oak";
                            else if (s.material == "mountain" || s.material == "snow")
                                d.type = "rock_large";
                            else if (s.material == "desert")
                                d.type = "cactus";
                            else
                                d.type = "grass_tuft";
                            chunk.decorations.push_back(d);
                        }
                    }
                }
            }
        }
        return chunk;
    }
};

// ── WorldChunkGen ─────────────────────────────────────────────────────────
WorldChunkGen::WorldChunkGen() : m_impl(new Impl()) {}
WorldChunkGen::WorldChunkGen(const WorldChunkConfig& cfg) : m_impl(new Impl()) {
    m_impl->cfg = cfg;
}
WorldChunkGen::~WorldChunkGen() { delete m_impl; }

void WorldChunkGen::SetConfig(const WorldChunkConfig& cfg) {
    m_impl->cfg = cfg;
}
const WorldChunkConfig& WorldChunkGen::Config() const { return m_impl->cfg; }

const WorldChunk& WorldChunkGen::GenerateChunk(int32_t cx, int32_t cz) {
    ChunkCoord key{cx, cz};
    auto it = m_impl->cache.find(key);
    if (it != m_impl->cache.end()) {
        // Move to front of LRU list
        m_impl->lruList.splice(m_impl->lruList.begin(),
                               m_impl->lruList, it->second.iter);
        return it->second.chunk;
    }

    // Evict oldest if at capacity
    if (m_impl->cache.size() >= m_impl->cfg.maxCacheSize &&
        !m_impl->lruList.empty()) {
        m_impl->cache.erase(m_impl->lruList.back());
        m_impl->lruList.pop_back();
    }

    m_impl->lruList.push_front(key);
    CacheEntry& entry = m_impl->cache[key];
    entry.chunk = m_impl->generate(cx, cz);
    entry.iter  = m_impl->lruList.begin();

    for (auto& cb : m_impl->callbacks) cb(entry.chunk);
    return entry.chunk;
}

std::vector<ChunkCoord> WorldChunkGen::LoadRadius(int32_t cx, int32_t cz,
                                                   uint32_t radius) {
    std::vector<ChunkCoord> loaded;
    for (int32_t dz = -static_cast<int32_t>(radius);
         dz <= static_cast<int32_t>(radius); ++dz) {
        for (int32_t dx = -static_cast<int32_t>(radius);
             dx <= static_cast<int32_t>(radius); ++dx) {
            GenerateChunk(cx + dx, cz + dz);
            loaded.push_back({cx + dx, cz + dz});
        }
    }
    return loaded;
}

bool WorldChunkGen::IsCached(int32_t cx, int32_t cz) const {
    return m_impl->cache.count({cx, cz}) > 0;
}

void WorldChunkGen::EvictChunk(int32_t cx, int32_t cz) {
    ChunkCoord key{cx, cz};
    auto it = m_impl->cache.find(key);
    if (it != m_impl->cache.end()) {
        m_impl->lruList.erase(it->second.iter);
        m_impl->cache.erase(it);
    }
}

void WorldChunkGen::ClearCache() {
    m_impl->cache.clear();
    m_impl->lruList.clear();
}

size_t WorldChunkGen::CacheSize() const { return m_impl->cache.size(); }

void WorldChunkGen::OnChunkReady(ChunkReadyCb cb) {
    m_impl->callbacks.push_back(std::move(cb));
}

} // namespace PCG
