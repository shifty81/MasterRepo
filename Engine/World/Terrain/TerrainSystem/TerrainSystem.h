#pragma once
/**
 * @file TerrainSystem.h
 * @brief Heightmap terrain: load, sample, deform, normal, splat, erosion.
 *
 * Features:
 *   - LoadHeightmap(id, data[], width, height, scale) → bool: grayscale float[0,1]
 *   - GetHeight(id, worldX, worldZ) → float: bilinear sample
 *   - GetNormal(id, worldX, worldZ, outNX, outNY, outNZ)
 *   - SetHeight(id, worldX, worldZ, h): modify heightmap
 *   - DeformSphere(id, cx, cz, radius, strength): raise/lower hemisphere
 *   - FlattenArea(id, cx, cz, radius, targetH)
 *   - SmoothArea(id, cx, cz, radius, iterations)
 *   - HydraulicErosion(id, iterations, rainAmount): simulate rainfall erosion
 *   - SetSplatmap(id, layer, weights[]): 4-channel splat texture weights
 *   - GetSplatWeight(id, layer, worldX, worldZ) → float
 *   - GetWidth(id) / GetHeight(id) → uint32_t: map dimensions
 *   - GetWorldScale(id) → float
 *   - ExportHeightmap(id, out[]) → uint32_t: fills float array
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <vector>

namespace Engine {

class TerrainSystem {
public:
    TerrainSystem();
    ~TerrainSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Load
    bool LoadHeightmap(uint32_t id, const std::vector<float>& data,
                       uint32_t width, uint32_t height, float scale = 1.f);

    // Sample
    float GetHeight(uint32_t id, float worldX, float worldZ) const;
    void  GetNormal(uint32_t id, float worldX, float worldZ,
                    float& nx, float& ny, float& nz) const;

    // Deform
    void SetHeight   (uint32_t id, float worldX, float worldZ, float h);
    void DeformSphere(uint32_t id, float cx, float cz,
                      float radius, float strength);
    void FlattenArea (uint32_t id, float cx, float cz,
                      float radius, float targetH);
    void SmoothArea  (uint32_t id, float cx, float cz,
                      float radius, uint32_t iterations = 1);

    // Erosion
    void HydraulicErosion(uint32_t id, uint32_t iterations,
                          float rainAmount = 0.01f);

    // Splat
    void  SetSplatmap (uint32_t id, uint32_t layer,
                       const std::vector<float>& weights);
    float GetSplatWeight(uint32_t id, uint32_t layer,
                         float worldX, float worldZ) const;

    // Query
    uint32_t GetMapWidth  (uint32_t id) const;
    uint32_t GetMapHeight (uint32_t id) const;
    float    GetWorldScale(uint32_t id) const;
    uint32_t ExportHeightmap(uint32_t id, std::vector<float>& out) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
