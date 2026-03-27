#pragma once
/**
 * @file WaterRipple.h
 * @brief Height-field water ripple simulation with splash input, damping, and normal generation.
 *
 * Features:
 *   - Init(width, height, cellSize): allocate two ping-pong height buffers
 *   - Splash(worldX, worldY, force): add impulse displacement at grid cell
 *   - Update(dt): propagate ripples using finite-difference wave equation
 *   - SetDamping(d): per-step energy attenuation [0,1], default 0.99
 *   - SetSpeed(c): wave propagation speed (cells/sec), default 8.0
 *   - GetHeight(worldX, worldY) → float: current displacement at world position
 *   - GetHeightBuffer() → const float*: raw grid for GPU upload
 *   - GenerateNormals(outNormalsXYZ): compute surface normals from height differences
 *   - SetBoundaryMode(clamp/wrap/absorb): edge condition
 *   - Reset(): zero all heights
 */

#include <cstdint>

namespace Engine {

enum class WaterBoundary : uint8_t { Clamp, Wrap, Absorb };

class WaterRipple {
public:
    WaterRipple();
    ~WaterRipple();

    void Init    (uint32_t gridW, uint32_t gridH, float cellSize=0.25f);
    void Shutdown();
    void Reset   ();

    void Update(float dt);
    void Splash(float worldX, float worldY, float force);

    float        GetHeight      (float worldX, float worldY) const;
    const float* GetHeightBuffer() const;    ///< row-major, size = gridW*gridH

    // Normal map generation: outNormals must be 3*gridW*gridH floats (XYZ per cell)
    void GenerateNormals(float* outNormals) const;

    void SetDamping     (float d);
    void SetSpeed       (float c);
    void SetBoundaryMode(WaterBoundary mode);

    uint32_t GridWidth()  const;
    uint32_t GridHeight() const;
    float    CellSize()   const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
