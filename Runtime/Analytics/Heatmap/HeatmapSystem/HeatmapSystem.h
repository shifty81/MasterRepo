#pragma once
/**
 * @file HeatmapSystem.h
 * @brief 2D/3D heat-map accumulation, Gaussian splat, normalise, PNG/CSV export.
 *
 * Features:
 *   - 2D grid heatmap: world AABB → grid cell mapping
 *   - Splat: AddSample(worldPos, weight) — Gaussian kernel radius configurable
 *   - Normalise: map raw values to [0,1]
 *   - Multiple named layers (deaths, pickups, damage, etc.)
 *   - QueryValue(worldPos) → normalised intensity at position
 *   - GetCell(x,y) → raw accumulation
 *   - Decay: optional per-tick exponential decay
 *   - Reset layer
 *   - PNG export: writes greyscale (or heat-colour mapped) image
 *   - CSV export: raw grid values
 *   - Thread-safe accumulation via mutex
 *
 * Typical usage:
 * @code
 *   HeatmapSystem hm;
 *   HeatmapDesc d; d.worldMin={-50,-50}; d.worldMax={50,50};
 *   d.gridW=128; d.gridH=128; d.splatRadius=2.f;
 *   uint32_t layer = hm.CreateLayer("deaths", d);
 *   hm.AddSample(layer, deathPos, 1.f);
 *   hm.Normalise(layer);
 *   float v = hm.QueryValue(layer, worldPos);
 *   hm.ExportPNG(layer, "deaths.png");
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct HeatmapDesc {
    float    worldMin[2]{-100,-100};
    float    worldMax[2]{ 100, 100};
    uint32_t gridW  {128};
    uint32_t gridH  {128};
    float    splatRadius{3.f};       ///< Gaussian sigma in world units
    float    decayRate  {0.f};       ///< 0 = no decay; >0 = per-second exponential
};

class HeatmapSystem {
public:
    HeatmapSystem();
    ~HeatmapSystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);         ///< applies decay to all layers

    // Layer management
    uint32_t CreateLayer (const std::string& name, const HeatmapDesc& desc);
    void     DestroyLayer(uint32_t layerId);
    bool     HasLayer    (uint32_t layerId)     const;
    uint32_t FindLayer   (const std::string& name) const;
    std::vector<uint32_t> GetAllLayers()        const;

    // Accumulation
    void AddSample  (uint32_t layerId, const float worldPos2[2], float weight=1.f);
    void AddSample3D(uint32_t layerId, const float worldPos3[3], float weight=1.f);
    void ResetLayer (uint32_t layerId);

    // Query
    float    QueryValue(uint32_t layerId, const float worldPos2[2]) const;
    float    GetCell   (uint32_t layerId, uint32_t x, uint32_t y)   const;
    float    MaxRaw    (uint32_t layerId)                            const;
    void     Normalise (uint32_t layerId);
    const std::vector<float>& GetGrid(uint32_t layerId) const;

    // Export
    bool ExportPNG(uint32_t layerId, const std::string& path,
                   bool heatColour=true) const;
    bool ExportCSV(uint32_t layerId, const std::string& path) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
