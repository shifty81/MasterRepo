#pragma once
/**
 * @file SpawnManager.h
 * @brief Wave-based entity spawner with pools, budgets, spawn-point registry, and triggers.
 *
 * Features:
 *   - RegisterSpawnPoint(id, pos, tags[]): add a spawn location with optional tags
 *   - RegisterEntityType(typeId, poolSize): define a poolable entity type
 *   - CreateWave(waveDesc) → waveId: define a spawn wave
 *   - StartWave(waveId): begin spawning the wave
 *   - Tick(dt): advance spawn timers, call OnSpawn callback for each entity
 *   - IsWaveComplete(waveId) → bool
 *   - SetMaxConcurrent(n): global live entity cap
 *   - GetAliveCount() → uint32_t
 *   - OnEntityDead(entityId): return entity to pool, update alive count
 *   - SetOnSpawn(cb): callback(entityTypeId, spawnPos) → entityId
 *   - SetOnWaveComplete(waveId, cb)
 *   - SetOnAllWavesComplete(cb)
 *   - GetSpawnPointsByTag(tag) → ids[]
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct SpawnVec3 { float x, y, z; };

struct WaveEntry {
    uint32_t typeId;
    uint32_t count;
    float    intervalSec{0.5f}; ///< delay between individual spawns
    float    delayFromWaveStart{0.f};
    std::string spawnTag;       ///< optional: filter spawn-points by tag
};

struct WaveDesc {
    std::string            name;
    std::vector<WaveEntry> entries;
    bool                   repeat{false};
};

class SpawnManager {
public:
    SpawnManager();
    ~SpawnManager();

    void Init    ();
    void Shutdown();
    void Reset   ();
    void Tick    (float dt);

    // Registry
    void RegisterSpawnPoint (uint32_t spawnPtId, SpawnVec3 pos,
                             const std::vector<std::string>& tags={});
    void RegisterEntityType (uint32_t typeId, uint32_t poolSize=10);

    // Wave authoring & control
    uint32_t CreateWave     (const WaveDesc& desc);
    void     StartWave      (uint32_t waveId);
    void     StopWave       (uint32_t waveId);
    bool     IsWaveComplete (uint32_t waveId) const;

    // Entity lifecycle
    void     OnEntityDead   (uint32_t entityId);

    // Config
    void     SetMaxConcurrent(uint32_t n);
    uint32_t GetAliveCount   () const;

    // Spawn-point query
    std::vector<uint32_t> GetSpawnPointsByTag(const std::string& tag) const;
    SpawnVec3             GetSpawnPointPos   (uint32_t spawnPtId)     const;

    // Callbacks
    void SetOnSpawn          (std::function<uint32_t(uint32_t typeId, SpawnVec3 pos)> cb);
    void SetOnWaveComplete   (uint32_t waveId, std::function<void()> cb);
    void SetOnAllWavesComplete(std::function<void()> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
