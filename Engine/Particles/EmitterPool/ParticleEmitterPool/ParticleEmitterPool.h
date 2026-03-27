#pragma once
/**
 * @file ParticleEmitterPool.h
 * @brief Pooled particle emitter management: spawn, recycle, burst, limit, LOD.
 *
 * Features:
 *   - EmitterDef: id, maxParticles, spawnRate, lifetime, speed, spread, color
 *   - RegisterEmitter(def) → bool
 *   - SpawnEmitter(defId, x, y, z) → instanceId: acquire from pool
 *   - ReturnEmitter(instanceId): return to pool
 *   - SetEmitterPosition(instanceId, x, y, z)
 *   - SetEmitterActive(instanceId, on)
 *   - Burst(instanceId, count): instant particle burst
 *   - Tick(dt): update all active emitters and particles
 *   - GetActiveCount() → uint32_t: active emitter instances
 *   - GetPoolSize(defId) → uint32_t: total pooled instances
 *   - SetMaxActiveEmitters(n)
 *   - SetLODDistances(near, far): fade/disable emitters by distance
 *   - SetListenerPos(x, y, z): for LOD distance calc
 *   - SetOnSpawn(defId, cb) / SetOnReturn(defId, cb)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>

namespace Engine {

struct EmitterDef {
    uint32_t id;
    uint32_t maxParticles{100};
    float    spawnRate   {10.f};
    float    lifetime    {2.f};
    float    speed       {1.f};
    float    spread      {0.3f};
    uint32_t poolSize    {8};
};

class ParticleEmitterPool {
public:
    ParticleEmitterPool();
    ~ParticleEmitterPool();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Registration
    bool RegisterEmitter(const EmitterDef& def);

    // Pool operations
    uint32_t SpawnEmitter  (uint32_t defId, float x, float y, float z);
    void     ReturnEmitter (uint32_t instanceId);

    // Instance control
    void SetEmitterPosition(uint32_t instanceId, float x, float y, float z);
    void SetEmitterActive  (uint32_t instanceId, bool on);
    void Burst             (uint32_t instanceId, uint32_t count);

    // Per-frame
    void Tick(float dt);

    // Config
    void SetMaxActiveEmitters(uint32_t n);
    void SetLODDistances     (float near, float far);
    void SetListenerPos      (float x, float y, float z);

    // Query
    uint32_t GetActiveCount(          ) const;
    uint32_t GetPoolSize   (uint32_t defId) const;

    // Callbacks
    void SetOnSpawn (uint32_t defId, std::function<void(uint32_t instanceId)> cb);
    void SetOnReturn(uint32_t defId, std::function<void(uint32_t instanceId)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
