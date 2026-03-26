#pragma once
/**
 * @file PlanetGenerator.h
 * @brief Procedural planet surface generator.
 *
 * Produces:
 *   - Spherical heightmap via fractional Brownian motion (fBm) layered on a
 *     cube-sphere layout
 *   - Biome distribution driven by elevation + latitude (temperature/moisture)
 *   - Atmosphere colour gradient
 *   - Texture maps (albedo, normal, roughness) at configurable resolution
 *   - Optional ring system parameters
 *
 * Typical usage:
 * @code
 *   PlanetGenerator gen;
 *   PlanetConfig cfg;
 *   cfg.seed        = 42;
 *   cfg.radius      = 6371.f;  // km
 *   cfg.oceanLevel  = 0.45f;
 *   cfg.mapResolution = 512;
 *   gen.Init(cfg);
 *   PlanetData planet = gen.Generate();
 *   gen.ExportHeightmap("Planets/earth_height.png");
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace PCG {

// ── Planet configuration ──────────────────────────────────────────────────────

struct PlanetConfig {
    uint64_t seed{0};
    float    radius{6371.f};        ///< planet radius in km
    float    oceanLevel{0.45f};     ///< fraction of surface below sea level
    float    roughness{0.6f};       ///< terrain roughness (0–1)
    uint32_t mapResolution{256};    ///< heightmap / texture resolution (pixels)
    uint32_t octaves{6};            ///< fBm octave count
    float    atmosphereDensity{1.f};///< 0 = no atmosphere
    float    atmosphereColour[3]{0.4f, 0.6f, 1.f}; ///< RGB sky tint
    bool     hasRings{false};
    float    ringInnerRadius{1.2f}; ///< in units of planet radius
    float    ringOuterRadius{2.0f};
    bool     hasIce{true};          ///< polar ice caps
    uint32_t biomeCount{6};         ///< number of distinct biome types
};

// ── Biome classification ──────────────────────────────────────────────────────

enum class BiomeType : uint8_t {
    Ocean = 0, DeepOcean, Beach, Desert, Grassland,
    Forest, Tundra, Arctic, Volcanic, COUNT
};

// ── Output heightmap row-major (mapResolution × mapResolution) ────────────────

struct PlanetData {
    uint32_t              resolution{0};
    std::vector<float>    heightmap;   ///< normalised 0–1 elevation
    std::vector<BiomeType> biomemap;   ///< per-texel biome
    std::vector<uint8_t>  albedoRGBA;  ///< RGBA albedo texture bytes
    float                 minHeight{0.f};
    float                 maxHeight{0.f};
    uint32_t              oceanTexelCount{0};
    uint32_t              landTexelCount{0};
    uint64_t              seed{0};
    std::string           errorMessage;
};

// ── PlanetGenerator ───────────────────────────────────────────────────────────

class PlanetGenerator {
public:
    PlanetGenerator();
    ~PlanetGenerator();

    void Init(const PlanetConfig& config = {});
    void Shutdown();

    // ── Generation ────────────────────────────────────────────────────────────

    PlanetData Generate();
    PlanetData GenerateWithSeed(uint64_t seed);

    // ── Export ────────────────────────────────────────────────────────────────

    bool ExportHeightmap(const std::string& path) const;
    bool ExportAlbedo(const std::string& path)    const;
    bool ExportBiomeMap(const std::string& path)  const;

    // ── Query ─────────────────────────────────────────────────────────────────

    PlanetData    LastResult()  const;
    PlanetConfig  GetConfig()   const;
    void          SetConfig(const PlanetConfig& config);

    float SampleHeight(float longitudeDeg, float latitudeDeg) const;
    BiomeType SampleBiome(float longitudeDeg, float latitudeDeg) const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnProgress(std::function<void(float pct)> cb);
    void OnComplete(std::function<void(const PlanetData&)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace PCG
