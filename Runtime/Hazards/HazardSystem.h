#pragma once
/**
 * @file HazardSystem.h
 * @brief Environmental hazards — radiation zones, asteroid fields, gravity wells, nebulae.
 *
 * Manages a collection of spatial hazard volumes.  On each Tick() the system
 * tests registered entities' positions against active hazards and fires
 * damage/effect callbacks.
 *
 * Hazard types:
 *   Radiation     — continuous HP drain, shields provide no protection
 *   AsteroidField — random collision events, structural damage to hull
 *   GravityWell   — acceleration towards a point, fuel/thruster drain
 *   Nebula        — reduced sensor range, passive shield drain
 *   IonStorm      — disables electronics, power drain per tick
 *
 * Usage:
 * @code
 *   HazardSystem hazards;
 *   hazards.Init();
 *   uint32_t rz = hazards.AddHazard({HazardType::Radiation, {0,0,500}, 200.f, 2.5f});
 *   hazards.RegisterEntity(playerEntityId, &playerPos);
 *   hazards.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>

namespace Runtime {

// ── Hazard type ───────────────────────────────────────────────────────────────

enum class HazardType : uint8_t {
    Radiation,
    AsteroidField,
    GravityWell,
    Nebula,
    IonStorm,
    COUNT
};

// ── Hazard zone descriptor ────────────────────────────────────────────────────

struct HazardZone {
    uint32_t   id{0};
    HazardType type{HazardType::Radiation};
    float      cx{0}, cy{0}, cz{0};  ///< centre
    float      radius{100.0f};
    float      intensity{1.0f};      ///< scales damage/effect magnitude
    std::string label;
    bool       active{true};
};

// ── Per-entity position pointer (caller owns the floats) ─────────────────────

struct EntityPos {
    uint32_t entityId{0};
    const float* x{nullptr};  ///< pointers into entity's transform
    const float* y{nullptr};
    const float* z{nullptr};
};

// ── Hazard effect events ──────────────────────────────────────────────────────

struct HazardEffect {
    uint32_t   entityId{0};
    uint32_t   hazardId{0};
    HazardType type{HazardType::Radiation};
    float      damagePerSec{0.0f};
    float      shieldDrainPerSec{0.0f};
    float      gravAccelX{0}, gravAccelZ{0};
    bool       sensorsJammed{false};
    bool       powerDrained{false};
};

using HazardEffectFn  = std::function<void(const HazardEffect&)>;
using EntityEnteredFn = std::function<void(uint32_t entityId, uint32_t hazardId)>;
using EntityExitedFn  = std::function<void(uint32_t entityId, uint32_t hazardId)>;

// ── HazardSystem ─────────────────────────────────────────────────────────────

class HazardSystem {
public:
    void Init();

    // ── Hazard management ─────────────────────────────────────────────────
    uint32_t AddHazard(const HazardZone& zone);
    void     RemoveHazard(uint32_t hazardId);
    void     SetHazardActive(uint32_t hazardId, bool active);
    HazardZone* GetHazard(uint32_t hazardId);
    std::vector<uint32_t> AllHazardIds() const;

    // ── Entity registration ───────────────────────────────────────────────
    /// Register an entity for hazard testing.
    /// The provided pointers must remain valid for the entity's lifetime.
    void RegisterEntity(uint32_t entityId, const float* x, const float* y, const float* z);
    void UnregisterEntity(uint32_t entityId);

    // ── Tick ──────────────────────────────────────────────────────────────
    void Tick(float dt);

    // ── Queries ───────────────────────────────────────────────────────────
    bool IsInHazard(uint32_t entityId, uint32_t hazardId) const;
    std::vector<uint32_t> GetHazardsAt(float x, float y, float z) const;
    std::vector<uint32_t> GetActiveHazardsForEntity(uint32_t entityId) const;

    // ── Callbacks ─────────────────────────────────────────────────────────
    void SetEffectCallback(HazardEffectFn fn);
    void SetEnteredCallback(EntityEnteredFn fn);
    void SetExitedCallback(EntityExitedFn fn);

    size_t HazardCount()         const;
    size_t RegisteredEntityCount() const;

private:
    bool InsideZone(const HazardZone& zone, float x, float y, float pz) const;
    HazardEffect ComputeEffect(const HazardZone& zone, uint32_t entityId, float dt) const;

    uint32_t m_nextId{1};
    std::unordered_map<uint32_t, HazardZone> m_zones;
    std::vector<EntityPos>                   m_entities;

    /// Track which (entity, hazard) pairs are currently inside — for enter/exit events
    std::unordered_map<uint64_t, bool> m_inside; // key = entityId<<32 | hazardId

    HazardEffectFn  m_onEffect;
    EntityEnteredFn m_onEntered;
    EntityExitedFn  m_onExited;
};

} // namespace Runtime
