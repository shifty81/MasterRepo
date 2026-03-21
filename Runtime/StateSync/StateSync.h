#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace Runtime {

/// Snapshot of one entity's synchronised state.
struct EntityState {
    uint32_t    entityId{0};
    std::string componentType;  ///< e.g. "Transform", "Health", "Inventory"
    std::vector<uint8_t> payload; ///< Serialised component data
    uint64_t    tick{0};         ///< Simulation tick this snapshot belongs to
};

/// Full world-state snapshot for one tick.
struct WorldSnapshot {
    uint64_t                  tick{0};
    std::vector<EntityState>  entities;
};

using SnapshotCallback = std::function<void(const WorldSnapshot&)>;
using ConflictCallback = std::function<void(const EntityState& local,
                                             const EntityState& remote)>;

/// StateSync — deterministic state synchronisation for multiplayer / replay.
///
/// Records local world snapshots and reconciles them with remote snapshots
/// received over the network.  Desync detection fires the ConflictCallback
/// so the application can choose the resolution strategy (rollback / blend).
class StateSync {
public:
    StateSync();
    ~StateSync();

    // ── configuration ─────────────────────────────────────────
    /// Number of historical snapshots to retain for rollback (default 60).
    void SetRollbackWindow(uint32_t ticks);
    /// Tolerated byte-level diff before a desync is flagged (default 0 = exact).
    void SetDesyncTolerance(uint32_t bytes);

    // ── snapshot management ───────────────────────────────────
    /// Record a local snapshot for the current tick.
    void Capture(const WorldSnapshot& snap);
    /// Ingest a snapshot received from a remote peer.
    void ApplyRemote(const WorldSnapshot& remote);
    /// Roll back to the last clean snapshot at or before the given tick.
    bool Rollback(uint64_t toTick);

    // ── query ─────────────────────────────────────────────────
    uint64_t      CurrentTick()  const;
    WorldSnapshot GetSnapshot(uint64_t tick) const;
    bool          IsInSync()     const;
    uint32_t      DesyncCount()  const;

    // ── callbacks ─────────────────────────────────────────────
    void OnSnapshot(SnapshotCallback cb);
    void OnDesync(ConflictCallback cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
