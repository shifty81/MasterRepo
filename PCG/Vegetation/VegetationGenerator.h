#pragma once
/**
 * @file VegetationGenerator.h
 * @brief Biome-aware foliage and tree placement generator.
 *
 * VegetationGenerator takes a world grid (cells with biome and height data)
 * and places vegetation instances (trees, bushes, grass) using:
 *   - Per-biome density parameters from BiomeDef.treeDensity / rockDensity
 *   - Noise-modulated cluster density (reuses PCG::NoiseGenerator concepts)
 *   - Slope culling (steep terrain gets no vegetation)
 *   - Deterministic seeding for reproducible world gen
 *
 * Output is a flat list of VegetationInstance records consumed by the
 * scene builder / renderer.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace PCG {

// ── Vegetation asset descriptor ───────────────────────────────────────────
struct VegetationAsset {
    std::string assetID;    ///< Mesh/prefab key
    std::string tag;        ///< Biome tag e.g. "tree_pine"
    float       minScale{0.8f};
    float       maxScale{1.2f};
    float       weight{1.0f};  ///< Relative spawn probability
};

// ── Placed instance ───────────────────────────────────────────────────────
struct VegetationInstance {
    std::string assetID;
    float       x{0}, y{0}, z{0};  ///< World position
    float       rotY{0};           ///< Y-axis rotation (radians)
    float       scale{1.0f};
    std::string biomeID;
    std::string tag;
};

// ── Input cell (compatible with BiomeCell) ─────────────────────────────────
struct VegetationCell {
    int         x{0}, z{0};
    std::string biomeID;
    float       height{0.0f};
    float       slope{0.0f};    ///< Terrain slope [0,1]
    float       moisture{0.5f};
};

// ── Per-biome density config ──────────────────────────────────────────────
struct BiomeVegetationConfig {
    std::string                  biomeID;
    std::vector<VegetationAsset> assets;
    float                        density{0.1f};    ///< Base instances per cell
    float                        maxSlope{0.4f};   ///< Cells steeper than this get nothing
    float                        clusterScale{3.0f};///< Noise frequency for clustering
};

using VegetationCallback = std::function<void(const VegetationInstance&)>;

/// VegetationGenerator — biome-driven vegetation placement.
class VegetationGenerator {
public:
    VegetationGenerator();
    ~VegetationGenerator();

    // ── biome configs ─────────────────────────────────────────
    void RegisterBiome(BiomeVegetationConfig cfg);
    bool HasBiome(const std::string& biomeID) const;
    size_t BiomeCount() const;

    // ── generation ────────────────────────────────────────────
    /// Generate vegetation for a list of world cells.
    std::vector<VegetationInstance> Generate(
        const std::vector<VegetationCell>& cells,
        uint64_t seed = 0);

    /// Stream instances as they are generated.
    std::vector<VegetationInstance> GenerateWithCallback(
        const std::vector<VegetationCell>& cells,
        VegetationCallback cb,
        uint64_t seed = 0);

    // ── query ─────────────────────────────────────────────────
    size_t LastGeneratedCount() const;

    // ── tuning ────────────────────────────────────────────────
    void SetGlobalDensityScale(float scale); ///< Multiplied on all densities
    float GlobalDensityScale() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace PCG
