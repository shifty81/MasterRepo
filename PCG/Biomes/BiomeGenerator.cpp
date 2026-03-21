#include "PCG/Biomes/BiomeGenerator.h"
#include <cstdint>

namespace PCG {

// ── deterministic hash ────────────────────────────────────────────────────────
static float Hash2(int x, int z, uint64_t seed) {
    uint64_t h = seed ^ (static_cast<uint64_t>(x) * 2654435761ULL)
                      ^ (static_cast<uint64_t>(z) * 2246822519ULL);
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;
    return static_cast<float>(h & 0xFFFFFF) / static_cast<float>(0xFFFFFF);
}

// ── Impl ─────────────────────────────────────────────────────────────────────
struct BiomeGenerator::Impl {
    std::vector<BiomeDef> biomes;
    uint64_t seed{12345};
    int width{64};
    int depth{64};
};

BiomeGenerator::BiomeGenerator() : m_impl(new Impl()) {}
BiomeGenerator::~BiomeGenerator() { delete m_impl; }

void BiomeGenerator::RegisterBiome(const BiomeDef& b) { m_impl->biomes.push_back(b); }
void BiomeGenerator::ClearBiomes()                     { m_impl->biomes.clear(); }
size_t BiomeGenerator::BiomeCount()              const { return m_impl->biomes.size(); }
void BiomeGenerator::SetSeed(uint64_t seed)            { m_impl->seed = seed; }
void BiomeGenerator::SetWorldSize(int w, int d)        { m_impl->width = w; m_impl->depth = d; }

const BiomeDef* BiomeGenerator::Classify(float moisture, float temperature) const {
    for (const auto& b : m_impl->biomes) {
        if (moisture    >= b.moistureMin     && moisture    <= b.moistureMax &&
            temperature >= b.temperatureMin  && temperature <= b.temperatureMax)
            return &b;
    }
    return m_impl->biomes.empty() ? nullptr : &m_impl->biomes[0];
}

std::vector<BiomeCell> BiomeGenerator::Generate() {
    std::vector<BiomeCell> result;
    result.reserve(static_cast<size_t>(m_impl->width) *
                   static_cast<size_t>(m_impl->depth));
    for (int z = 0; z < m_impl->depth; ++z) {
        for (int x = 0; x < m_impl->width; ++x) {
            float m = Hash2(x,      z,      m_impl->seed);
            float t = Hash2(x+9999, z+9999, m_impl->seed ^ 0xABCDEFULL);
            const BiomeDef* biome = Classify(m, t);

            BiomeCell cell;
            cell.x           = x;
            cell.z           = z;
            cell.moisture    = m;
            cell.temperature = t;
            float h = Hash2(x+7777, z+7777, m_impl->seed ^ 0x123456ULL) * 2.0f - 1.0f;

            if (biome) {
                cell.biomeId = biome->id;
                cell.height  = h * biome->heightScale;
                if (Hash2(x+111, z+222, m_impl->seed ^ 0x55AAULL) < biome->treeDensity)
                    for (auto& tag : biome->vegetationTags) cell.spawnedTags.push_back(tag);
                if (Hash2(x+333, z+444, m_impl->seed ^ 0xAA55ULL) < biome->rockDensity)
                    for (auto& tag : biome->rockTags) cell.spawnedTags.push_back(tag);
            } else {
                cell.height = h;
            }
            result.push_back(cell);
        }
    }
    return result;
}

void BiomeGenerator::GenerateStreamed(BiomeCellCallback cb) {
    for (int z = 0; z < m_impl->depth; ++z) {
        for (int x = 0; x < m_impl->width; ++x) {
            float m = Hash2(x,      z,      m_impl->seed);
            float t = Hash2(x+9999, z+9999, m_impl->seed ^ 0xABCDEFULL);
            const BiomeDef* biome = Classify(m, t);

            BiomeCell cell;
            cell.x           = x;
            cell.z           = z;
            cell.moisture    = m;
            cell.temperature = t;
            float h = Hash2(x+7777, z+7777, m_impl->seed ^ 0x123456ULL) * 2.0f - 1.0f;

            if (biome) {
                cell.biomeId = biome->id;
                cell.height  = h * biome->heightScale;
                if (Hash2(x+111, z+222, m_impl->seed ^ 0x55AAULL) < biome->treeDensity)
                    for (auto& tag : biome->vegetationTags) cell.spawnedTags.push_back(tag);
                if (Hash2(x+333, z+444, m_impl->seed ^ 0xAA55ULL) < biome->rockDensity)
                    for (auto& tag : biome->rockTags) cell.spawnedTags.push_back(tag);
            } else {
                cell.height = h;
            }
            cb(cell);
        }
    }
}

} // namespace PCG
