#pragma once
/**
 * @file DamageZone.h
 * @brief Persistent hazard zones that damage entities overlapping per tick.
 *
 * Features:
 *   - ZoneShape: Sphere, Box, Cylinder
 *   - AddZone(id, shape, cx,cy,cz, hw,hh,hd, dps, type) → bool
 *   - RemoveZone(id) / RemoveAll()
 *   - SetZoneEnabled(id, on)
 *   - Tick(dt, queryFn): queryFn returns entities inside AABB; apply damage*dt
 *   - SetDamagePerSecond(id, dps)
 *   - SetDamageType(id, type): element/type tag
 *   - GetZoneCount() → uint32_t
 *   - GetEntitiesInZone(id, out[]) → uint32_t: cached from last Tick
 *   - SetOnEnter(cb) / SetOnExit(cb) / SetOnDamage(cb)
 *   - MoveZone(id, cx,cy,cz): translate zone centre
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Runtime {

enum class DamageZoneShape : uint8_t { Sphere, Box, Cylinder };

class DamageZone {
public:
    DamageZone();
    ~DamageZone();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Zone management
    bool AddZone   (uint32_t id, DamageZoneShape shape,
                    float cx, float cy, float cz,
                    float hw, float hh, float hd,
                    float dps, uint32_t damageType = 0);
    void RemoveZone(uint32_t id);
    void RemoveAll ();
    void SetZoneEnabled(uint32_t id, bool on);

    // Per-frame — queryFn(cx,cy,cz,hw,hh,hd, outIds[]) → count
    using QueryFn = std::function<uint32_t(float,float,float,float,float,float,
                                            std::vector<uint32_t>&)>;
    void Tick(float dt, QueryFn queryFn);

    // Config
    void SetDamagePerSecond(uint32_t id, float dps);
    void SetDamageType     (uint32_t id, uint32_t type);
    void MoveZone          (uint32_t id, float cx, float cy, float cz);

    // Query
    uint32_t GetZoneCount          () const;
    uint32_t GetEntitiesInZone     (uint32_t id, std::vector<uint32_t>& out) const;

    // Callbacks
    void SetOnEnter (std::function<void(uint32_t zoneId, uint32_t entityId)> cb);
    void SetOnExit  (std::function<void(uint32_t zoneId, uint32_t entityId)> cb);
    void SetOnDamage(std::function<void(uint32_t entityId, float dmg,
                                        uint32_t type)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
