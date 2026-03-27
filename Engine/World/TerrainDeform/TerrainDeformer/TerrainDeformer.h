#pragma once
/**
 * @file TerrainDeformer.h
 * @brief Runtime height-field terrain with brush deformation, erosion, and height queries.
 *
 * Features:
 *   - Init(width, depth, cellSize): allocate height grid
 *   - SetHeight(x, z, h) / GetHeight(x, z) → float: direct cell access
 *   - Raise/LowerBrush(worldX, worldZ, radius, strength, dt): additive/subtractive sculpt
 *   - SmoothBrush(worldX, worldZ, radius, strength, dt): Gaussian blur pass over region
 *   - FlattenBrush(worldX, worldZ, radius, targetHeight, strength, dt)
 *   - GetHeightAt(worldX, worldZ) → float: bilinear interpolation
 *   - GetNormalAt(worldX, worldZ) → Vec3: surface normal via finite diff
 *   - ApplyErosion(iterations, rainAmount, evaporation): hydraulic erosion pass
 *   - GetDirtyRegion(minX, minZ, maxX, maxZ): extents of cells changed since last clear
 *   - ClearDirty()
 *   - SaveHeightmap(path) / LoadHeightmap(path)
 *   - GetWidth() / GetDepth() / GetCellSize()
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Engine {

struct TDVec3 { float x, y, z; };

class TerrainDeformer {
public:
    TerrainDeformer();
    ~TerrainDeformer();

    void Init    (uint32_t width, uint32_t depth, float cellSize=1.f);
    void Shutdown();
    void Reset   ();

    // Direct access
    void  SetHeight(uint32_t x, uint32_t z, float h);
    float GetHeight(uint32_t x, uint32_t z) const;

    // Brush operations
    void RaiseBrush  (float wx, float wz, float radius, float strength, float dt);
    void LowerBrush  (float wx, float wz, float radius, float strength, float dt);
    void SmoothBrush (float wx, float wz, float radius, float strength, float dt);
    void FlattenBrush(float wx, float wz, float radius, float targetH, float strength, float dt);

    // Interpolated queries
    float  GetHeightAt(float wx, float wz) const;
    TDVec3 GetNormalAt(float wx, float wz) const;

    // Simulation
    void ApplyErosion(uint32_t iterations=50, float rain=0.01f, float evaporation=0.05f);

    // Dirty tracking
    void GetDirtyRegion(uint32_t& minX, uint32_t& minZ, uint32_t& maxX, uint32_t& maxZ) const;
    void ClearDirty();
    bool IsDirty() const;

    // Serialisation
    bool SaveHeightmap(const std::string& path) const;
    bool LoadHeightmap(const std::string& path);

    // Dimensions
    uint32_t GetWidth   () const;
    uint32_t GetDepth   () const;
    float    GetCellSize() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
