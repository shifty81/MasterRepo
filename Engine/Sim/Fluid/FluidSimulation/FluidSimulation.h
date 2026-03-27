#pragma once
/**
 * @file FluidSimulation.h
 * @brief Grid-based Eulerian fluid sim: velocity advection, pressure projection, density.
 *
 * Features:
 *   - SetGridSize(w, h, d) / SetCellSize(s): configure simulation grid
 *   - SetViscosity(v) / SetDiffusion(d)
 *   - AddVelocity(x, y, z, vx, vy, vz): inject velocity at grid cell
 *   - AddDensity(x, y, z, amount): inject density (smoke/fluid)
 *   - Step(dt): advance simulation one frame
 *   - GetDensity(x, y, z) → float
 *   - GetVelocity(x, y, z) → Vec3
 *   - GetPressure(x, y, z) → float
 *   - SetObstacle(x, y, z, on): mark cell as solid boundary
 *   - Reset(): clear all fields
 *   - SetOnStep(cb): post-step callback
 *   - GetCellCount() → uint32_t (w*h*d)
 *   - CopyDensityField(out[]): write full density array
 *   - Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct FluidVec3 { float x, y, z; };

class FluidSimulation {
public:
    FluidSimulation();
    ~FluidSimulation();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Configuration
    void SetGridSize (uint32_t w, uint32_t h, uint32_t d);
    void SetCellSize (float s);
    void SetViscosity(float v);
    void SetDiffusion(float d);

    // Source terms
    void AddVelocity(uint32_t x, uint32_t y, uint32_t z,
                     float vx, float vy, float vz);
    void AddDensity (uint32_t x, uint32_t y, uint32_t z, float amount);

    // Obstacles
    void SetObstacle(uint32_t x, uint32_t y, uint32_t z, bool solid);

    // Per-frame
    void Step(float dt);

    // Query
    float     GetDensity (uint32_t x, uint32_t y, uint32_t z) const;
    FluidVec3 GetVelocity(uint32_t x, uint32_t y, uint32_t z) const;
    float     GetPressure(uint32_t x, uint32_t y, uint32_t z) const;
    uint32_t  GetCellCount() const;
    void      CopyDensityField(std::vector<float>& out) const;

    // Callback
    void SetOnStep(std::function<void(float dt)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
