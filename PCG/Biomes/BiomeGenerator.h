#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace PCG {

/// A biome definition: controls terrain parameters and asset spawning.
struct BiomeDef {
    std::string id;
    std::string name;

    // ── terrain shaping ───────────────────────────────────────
    float heightScale{1.0f};    ///< Multiplied onto the base heightfield
    float roughness{0.5f};      ///< 0.0 smooth → 1.0 very rough
    float moistureMin{0.0f};    ///< Moisture range this biome occupies [0,1]
    float moistureMax{1.0f};
    float temperatureMin{0.0f}; ///< Temperature range [0,1]
    float temperatureMax{1.0f};

    // ── asset tags spawned in this biome ─────────────────────
    std::vector<std::string> vegetationTags;
    std::vector<std::string> rockTags;
    std::vector<std::string> structureTags;
    float                   treeDensity{0.1f};
    float                   rockDensity{0.05f};
};

/// One world cell produced by the biome generator.
struct BiomeCell {
    int         x{0};
    int         z{0};
    std::string biomeId;
    float       height{0.0f};
    float       moisture{0.5f};
    float       temperature{0.5f};
    std::vector<std::string> spawnedTags; ///< Asset tags spawned here
};

using BiomeCellCallback = std::function<void(const BiomeCell&)>;

/// BiomeGenerator — biome-aware world generation.
///
/// Layered on top of PCG::HeightField / WorldNode.  Classifies each world
/// cell into a biome by mapping its moisture and temperature values
/// (derived from the heightfield + noise) to registered BiomeDef ranges.
/// Asset spawn lists are produced per-cell for the scene builder.
class BiomeGenerator {
public:
    BiomeGenerator();
    ~BiomeGenerator();

    // ── configuration ─────────────────────────────────────────
    void RegisterBiome(const BiomeDef& biome);
    void ClearBiomes();
    size_t BiomeCount() const;
    /// Noise seed used for moisture/temperature maps.
    void SetSeed(uint64_t seed);
    /// World grid dimensions.
    void SetWorldSize(int width, int depth);

    // ── generation ────────────────────────────────────────────
    /// Generate a flat grid of BiomeCells.  Iterates (0,0) to (width-1,depth-1).
    std::vector<BiomeCell> Generate();
    /// Same as Generate() but fires the callback for each cell as it is produced.
    void GenerateStreamed(BiomeCellCallback cb);

    // ── query ─────────────────────────────────────────────────
    /// Classify a single point by its moisture/temperature values.
    const BiomeDef* Classify(float moisture, float temperature) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace PCG
