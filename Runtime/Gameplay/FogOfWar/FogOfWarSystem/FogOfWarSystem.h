#pragma once
/**
 * @file FogOfWarSystem.h
 * @brief 2D grid-based fog of war with per-entity vision radius, decay, and shroud query.
 *
 * Features:
 *   - Init(width, height, cellSize): create revealed/explored float grids
 *   - RegisterObserver(id, radius): entity that reveals fog in a circle
 *   - UnregisterObserver(id)
 *   - Update(observers' world positions): re-compute revealed cells per frame
 *   - IsVisible(worldX, worldY) → bool: true if currently in a revealed cell
 *   - IsExplored(worldX, worldY) → bool: once-revealed (shroud) query
 *   - GetRevealedGrid() / GetExploredGrid() → const float* for rendering
 *   - Decay mode: revealed cells fade back to fog over time (configurable rate)
 *   - SetDecayRate(r): cells per second fade from 1→0 when no observer covers them
 *   - RevealArea(cx, cy, radius): one-shot static reveal (e.g. exploration trigger)
 *   - Reset(): clears all grids and observers
 */

#include <cstdint>
#include <string>
#include <unordered_map>

namespace Runtime {

class FogOfWarSystem {
public:
    FogOfWarSystem();
    ~FogOfWarSystem();

    void Init    (uint32_t gridW, uint32_t gridH, float cellSize=1.f);
    void Shutdown();
    void Reset   ();

    // Observer registration (entity id → vision radius in world units)
    void RegisterObserver  (uint32_t entityId, float radius);
    void UnregisterObserver(uint32_t entityId);
    void SetObserverPos    (uint32_t entityId, float worldX, float worldY);

    // Simulation
    void Update(float dt);

    // Query
    bool IsVisible (float worldX, float worldY) const;
    bool IsExplored(float worldX, float worldY) const;

    // Raw grid access for renderer (row-major, values [0,1])
    const float* GetRevealedGrid()  const;
    const float* GetExploredGrid()  const;
    uint32_t     GridWidth()        const;
    uint32_t     GridHeight()       const;
    float        CellSize()         const;

    // Manual reveal
    void RevealArea(float worldX, float worldY, float radius);

    // Config
    void SetDecayRate(float ratePerSec); ///< 0 = no decay (classic fog of war)
    void SetFalloffExponent(float e);    ///< circle edge softness

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
