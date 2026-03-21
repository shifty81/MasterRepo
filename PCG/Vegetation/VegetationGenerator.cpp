#include "PCG/Vegetation/VegetationGenerator.h"
#include <cmath>
#include <algorithm>
#include <ctime>

namespace PCG {

// ── Deterministic LCG ─────────────────────────────────────────────────────
static uint64_t lcgNext(uint64_t& s) {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    return s;
}
static float lcgFloat(uint64_t& s) {
    return static_cast<float>(lcgNext(s) >> 11) / static_cast<float>(1ULL << 53);
}
static float lcgRange(uint64_t& s, float lo, float hi) {
    return lo + lcgFloat(s) * (hi - lo);
}
// Simple 2D value noise in [0,1]
static float valueNoise(int x, int z, uint64_t seed) {
    uint64_t h = seed ^ (static_cast<uint64_t>(x) * 2654435761ULL)
                      ^ (static_cast<uint64_t>(z) * 2246822519ULL);
    h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ULL;
    h = (h ^ (h >> 27)) * 0x94d049bb133111ebULL;
    h ^= h >> 31;
    return static_cast<float>(h >> 11) / static_cast<float>(1ULL << 53);
}

// ── Impl ─────────────────────────────────────────────────────────────────
struct VegetationGenerator::Impl {
    std::unordered_map<std::string, BiomeVegetationConfig> biomes;
    float globalDensityScale{1.0f};
    size_t lastCount{0};
};

VegetationGenerator::VegetationGenerator() : m_impl(new Impl()) {}
VegetationGenerator::~VegetationGenerator() { delete m_impl; }

void VegetationGenerator::RegisterBiome(BiomeVegetationConfig cfg) {
    m_impl->biomes[cfg.biomeID] = std::move(cfg);
}
bool VegetationGenerator::HasBiome(const std::string& id) const {
    return m_impl->biomes.count(id) > 0;
}
size_t VegetationGenerator::BiomeCount() const { return m_impl->biomes.size(); }

std::vector<VegetationInstance> VegetationGenerator::Generate(
    const std::vector<VegetationCell>& cells, uint64_t seed)
{
    return GenerateWithCallback(cells, nullptr, seed);
}

std::vector<VegetationInstance> VegetationGenerator::GenerateWithCallback(
    const std::vector<VegetationCell>& cells,
    VegetationCallback cb,
    uint64_t seed)
{
    uint64_t rng = seed ? seed : static_cast<uint64_t>(std::time(nullptr));
    std::vector<VegetationInstance> out;

    for (const auto& cell : cells) {
        auto it = m_impl->biomes.find(cell.biomeID);
        if (it == m_impl->biomes.end()) continue;
        const auto& cfg = it->second;

        if (cell.slope > cfg.maxSlope) continue;

        // Noise-modulated density
        float noise = valueNoise(cell.x, cell.z, rng);
        float effectiveDensity = cfg.density * m_impl->globalDensityScale *
                                  (0.5f + noise * cfg.clusterScale * 0.5f);
        effectiveDensity = std::min(effectiveDensity, 20.0f); // cap

        // Number of instances for this cell (Poisson approximation via floor+rand)
        int count = static_cast<int>(effectiveDensity);
        float frac = effectiveDensity - static_cast<float>(count);
        if (lcgFloat(rng) < frac) ++count;

        if (cfg.assets.empty()) continue;

        // Build cumulative weight table
        float totalWeight = 0.0f;
        for (const auto& a : cfg.assets) totalWeight += a.weight;

        for (int i = 0; i < count; ++i) {
            // Pick asset by weight
            float r = lcgFloat(rng) * totalWeight;
            float cum = 0.0f;
            const VegetationAsset* chosen = &cfg.assets[0];
            for (const auto& a : cfg.assets) {
                cum += a.weight;
                if (r <= cum) { chosen = &a; break; }
            }

            VegetationInstance inst;
            inst.assetID = chosen->assetID;
            inst.tag     = chosen->tag;
            inst.biomeID = cell.biomeID;
            // Random offset within cell (assume cell size 1.0)
            inst.x = static_cast<float>(cell.x) + lcgFloat(rng);
            inst.y = cell.height;
            inst.z = static_cast<float>(cell.z) + lcgFloat(rng);
            inst.rotY  = lcgFloat(rng) * 6.2831853f; // 0..2π
            inst.scale = lcgRange(rng, chosen->minScale, chosen->maxScale);

            out.push_back(inst);
            if (cb) cb(inst);
        }
    }

    m_impl->lastCount = out.size();
    return out;
}

size_t VegetationGenerator::LastGeneratedCount() const {
    return m_impl->lastCount;
}

void VegetationGenerator::SetGlobalDensityScale(float scale) {
    m_impl->globalDensityScale = scale < 0.0f ? 0.0f : scale;
}
float VegetationGenerator::GlobalDensityScale() const {
    return m_impl->globalDensityScale;
}

} // namespace PCG
