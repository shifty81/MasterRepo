#pragma once
/**
 * @file FluidSimulation.h
 * @brief Grid-based Eulerian fluid simulation (Navier-Stokes, 2D or 3D).
 *
 * Supports incompressible fluid with:
 *   - Velocity advection (semi-Lagrangian)
 *   - Pressure projection (Gauss-Seidel)
 *   - Density / smoke / dye advection for visual rendering
 *   - Solid boundary conditions
 *   - External forces (gravity, wind, buoyancy)
 *
 * Typical usage (2D smoke):
 * @code
 *   FluidSimulation sim;
 *   FluidSimConfig cfg;
 *   cfg.gridW = 128; cfg.gridH = 128; cfg.gridD = 1;
 *   cfg.viscosity = 0.0001f;
 *   sim.Init(cfg);
 *   sim.AddDensitySource(64, 64, 0, 1.f);
 *   sim.AddVelocitySource(64, 64, 0, 0.f, -5.f, 0.f);
 *   for (int i = 0; i < 200; ++i) {
 *       sim.Step(0.016f);
 *   }
 *   auto density = sim.GetDensityGrid();
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

// ── Simulation configuration ──────────────────────────────────────────────────

struct FluidSimConfig {
    uint32_t gridW{64};
    uint32_t gridH{64};
    uint32_t gridD{1};          ///< 1 = 2D simulation
    float    cellSize{1.f};
    float    viscosity{0.001f};
    float    diffusion{0.0001f};
    float    gravity[3]{0.f, -9.8f, 0.f};
    uint32_t pressureIterations{20};
    bool     wrapBoundaries{false};
};

// ── Velocity field (flat array, interleaved xyz) ──────────────────────────────

struct FluidVelocityField {
    uint32_t            w{0}, h{0}, d{0};
    std::vector<float>  u;  ///< x-component, w*h*d floats
    std::vector<float>  v;  ///< y-component
    std::vector<float>  w_vel; ///< z-component
};

// ── FluidSimulation ───────────────────────────────────────────────────────────

class FluidSimulation {
public:
    FluidSimulation();
    ~FluidSimulation();

    void Init(const FluidSimConfig& config = {});
    void Shutdown();
    void Reset();

    // ── Sources ───────────────────────────────────────────────────────────────

    void AddDensitySource(uint32_t x, uint32_t y, uint32_t z, float amount);
    void AddVelocitySource(uint32_t x, uint32_t y, uint32_t z,
                           float vx, float vy, float vz);
    void SetSolidCell(uint32_t x, uint32_t y, uint32_t z, bool solid);

    // ── External forces ───────────────────────────────────────────────────────

    void SetGravity(float gx, float gy, float gz);
    void AddGlobalForce(float fx, float fy, float fz);

    // ── Simulation step ───────────────────────────────────────────────────────

    void Step(float dt);

    // ── Queries ───────────────────────────────────────────────────────────────

    const std::vector<float>&   GetDensityGrid()  const;
    const FluidVelocityField&   GetVelocityField() const;
    FluidSimConfig              GetConfig()        const;

    float SampleDensity(uint32_t x, uint32_t y, uint32_t z) const;
    void  SampleVelocity(uint32_t x, uint32_t y, uint32_t z,
                          float& vx, float& vy, float& vz) const;

    // ── Export ────────────────────────────────────────────────────────────────

    bool ExportDensityPGM(const std::string& path) const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnStepComplete(std::function<void(uint32_t stepIdx)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
