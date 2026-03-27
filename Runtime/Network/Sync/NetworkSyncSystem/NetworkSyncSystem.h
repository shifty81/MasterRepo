#pragma once
/**
 * @file NetworkSyncSystem.h
 * @brief Component-state snapshot interpolation/extrapolation for networked entities.
 *
 * Features:
 *   - RegisterEntity(id): start tracking snapshots for a network entity
 *   - UnregisterEntity(id)
 *   - PushSnapshot(entityId, time, state): record a received authoritative state
 *   - GetInterpolatedState(entityId, renderTime) → SyncState: lerp between two surrounding snapshots
 *   - GetExtrapolatedState(entityId, renderTime) → SyncState: project beyond last snapshot
 *   - SetInterpolationDelay(seconds): buffer time behind server (default 0.1 s)
 *   - Tick(dt): advance render clock
 *   - GetRenderTime() → double
 *   - IsEntityRegistered(id) → bool
 *   - GetSnapshotCount(entityId) → uint32_t: buffered snapshot count
 *   - PurgeOldSnapshots(beforeTime): discard snapshots older than given server time
 *   - SetOnDesync(cb): callback when snapshot buffer runs dry (extrapolation forced)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Runtime {

struct SyncState {
    float pos[3]  {0,0,0};
    float rot[4]  {0,0,0,1}; ///< quaternion xyzw
    float vel[3]  {0,0,0};
    bool  active  {true};
};

class NetworkSyncSystem {
public:
    NetworkSyncSystem();
    ~NetworkSyncSystem();

    void Init    (float interpolationDelaySec=0.1f);
    void Shutdown();
    void Reset   ();
    void Tick    (float dt);

    // Entity management
    void RegisterEntity  (uint32_t id);
    void UnregisterEntity(uint32_t id);
    bool IsEntityRegistered(uint32_t id) const;

    // Snapshot ingestion
    void PushSnapshot(uint32_t entityId, double serverTime, const SyncState& state);

    // State query
    SyncState GetInterpolatedState (uint32_t entityId, double renderTime) const;
    SyncState GetExtrapolatedState (uint32_t entityId, double renderTime) const;

    // Config
    void   SetInterpolationDelay(float seconds);
    double GetRenderTime        () const;

    // Diagnostics
    uint32_t GetSnapshotCount(uint32_t entityId) const;
    void     PurgeOldSnapshots(double beforeTime);

    // Desync callback
    void SetOnDesync(std::function<void(uint32_t entityId)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
