#pragma once
/**
 * @file WorldGenerator.h
 * @brief Top-level world assembly: biomes, height map, feature placement, seed-based.
 *
 * Features:
 *   - Height map generation: Perlin/Simplex noise with octaves, persistence, lacunarity
 *   - Biome assignment: temperature/humidity grid → biome type per cell
 *   - Biome types: Ocean, Desert, Grassland, Forest, Tundra, Mountain, Swamp, Volcano (extensible)
 *   - Feature placement: spawn points, dungeons, towns, resources — density per biome
 *   - River system: source points → downhill flow → carved paths
 *   - Road network: connect feature locations with minimum-spanning-tree
 *   - Seed-reproducible: uint64_t seed
 *   - Multi-resolution: tile size configurable; world size in tiles
 *   - Progress callback for async generation
 *   - JSON export: heightmap + biome + feature lists
 *   - Chunk-friendly output: GetRegion(x,y,w,h) returns sub-grid
 *
 * Typical usage:
 * @code
 *   WorldGenerator wg;
 *   WorldGenParams p; p.width=512; p.height=512; p.seed=1337;
 *   p.octaves=6; p.onProgress=[](float t){ Log(t); };
 *   auto world = wg.Generate(p);
 *   auto biome = world.GetBiome(128,256);
 *   auto features = world.GetFeatures(BiomeType::Forest);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace PCG {

enum class BiomeType : uint8_t {
    Ocean=0, Beach, Grassland, Forest, Desert, Tundra,
    Mountain, Swamp, Volcano, Jungle, Count
};

struct WorldFeature {
    float       x{0.f}, y{0.f};
    std::string type;          ///< "dungeon", "town", "resource", etc.
    BiomeType   biome{BiomeType::Grassland};
    float       importance{1.f};
};

struct WorldGenParams {
    uint32_t    width    {256};
    uint32_t    height   {256};
    uint64_t    seed     {0};
    float       tileSize {1.f};

    // Noise
    uint32_t    octaves     {6};
    float       persistence {0.5f};
    float       lacunarity  {2.0f};
    float       scale       {0.005f};

    // Biome thresholds
    float       seaLevel    {0.35f};
    float       mountainLevel{0.75f};

    // Features
    float       dungeon_density{0.001f};
    float       town_density   {0.002f};
    float       resource_density{0.005f};

    // Rivers
    uint32_t    riverCount  {5};

    std::function<void(float)> onProgress;
};

struct WorldData {
    uint32_t             width{0}, height{0};
    std::vector<float>   heightMap;        ///< normalised 0-1, row-major
    std::vector<float>   temperature;      ///< row-major
    std::vector<float>   humidity;         ///< row-major
    std::vector<BiomeType> biomeMap;       ///< row-major
    std::vector<WorldFeature> features;
    std::vector<std::vector<std::pair<int32_t,int32_t>>> rivers;
    std::vector<std::pair<uint32_t,uint32_t>> roads;  ///< feature index pairs

    float     GetHeight(int32_t x, int32_t y) const;
    BiomeType GetBiome (int32_t x, int32_t y) const;
    std::vector<const WorldFeature*> GetFeatures(BiomeType biome) const;
    std::vector<const WorldFeature*> GetFeaturesByType(const std::string& type) const;
    // Sub-region extraction
    WorldData GetRegion(uint32_t x, uint32_t y, uint32_t w, uint32_t h) const;
};

class WorldGenerator {
public:
    WorldGenerator();
    ~WorldGenerator();

    WorldData Generate(const WorldGenParams& params);

    bool SaveJSON(const WorldData& world, const std::string& path) const;
    bool LoadJSON(const std::string& path, WorldData& outWorld)    const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace PCG
