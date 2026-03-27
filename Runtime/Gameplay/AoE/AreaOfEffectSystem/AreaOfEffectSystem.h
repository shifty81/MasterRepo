#pragma once
/**
 * @file AreaOfEffectSystem.h
 * @brief Geometric AoE shapes: spheres, cylinders, cones, rings; overlap queries, tick damage.
 *
 * Features:
 *   - CreateAoE(id, shape, pos, params) → aoeId: register an active AoE zone
 *   - DestroyAoE(id)
 *   - SetPosition(id, pos) / SetRotation(id, yawDeg)
 *   - SetOnEnter(id, cb) / SetOnExit(id, cb) / SetOnTick(id, interval, cb)
 *   - Tick(dt): advance per-tick timers, track enter/exit, fire callbacks
 *   - Query(pos, outIds[]) → uint32_t: which AoE zones contain the given point
 *   - GetOverlappingAgents(aoeId) → uint32_t[]: agents currently inside the AoE
 *   - RegisterAgent(agentId, pos)
 *   - UpdateAgent(agentId, pos)
 *   - UnregisterAgent(agentId)
 *   - SetRadius(id, r) / SetHeight(id, h) / SetAngle(id, deg): resize AoE
 *   - AoE shapes: Sphere, Cylinder, Cone, Ring
 *   - GetAoECount() → uint32_t
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class AoEShape : uint8_t { Sphere, Cylinder, Cone, Ring };

struct AoEVec3 { float x,y,z; };

struct AoEParams {
    float radius{3.f};
    float height{2.f};    ///< Cylinder/Cone
    float angle {60.f};   ///< Cone half-angle (degrees)
    float innerRadius{0}; ///< Ring inner radius
};

class AreaOfEffectSystem {
public:
    AreaOfEffectSystem();
    ~AreaOfEffectSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // AoE management
    uint32_t CreateAoE (uint32_t id, AoEShape shape,
                        AoEVec3 pos, const AoEParams& params);
    void     DestroyAoE(uint32_t id);

    // Shape control
    void SetPosition(uint32_t id, AoEVec3 pos);
    void SetRotation(uint32_t id, float yawDeg);
    void SetRadius  (uint32_t id, float r);
    void SetHeight  (uint32_t id, float h);
    void SetAngle   (uint32_t id, float deg);

    // Agent tracking
    void RegisterAgent  (uint32_t agentId, AoEVec3 pos);
    void UpdateAgent    (uint32_t agentId, AoEVec3 pos);
    void UnregisterAgent(uint32_t agentId);

    // Per-frame
    void Tick(float dt);

    // Queries
    uint32_t              Query             (AoEVec3 pos,
                                             std::vector<uint32_t>& outIds) const;
    std::vector<uint32_t> GetOverlappingAgents(uint32_t aoeId) const;
    uint32_t              GetAoECount       () const;

    // Callbacks
    void SetOnEnter(uint32_t id, std::function<void(uint32_t agentId)> cb);
    void SetOnExit (uint32_t id, std::function<void(uint32_t agentId)> cb);
    void SetOnTick (uint32_t id, float interval,
                    std::function<void(uint32_t agentId)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
