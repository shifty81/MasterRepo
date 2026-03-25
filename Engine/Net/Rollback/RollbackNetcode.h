#pragma once
/**
 * @file RollbackNetcode.h
 * @brief Rollback-based netcode for deterministic multiplayer synchronisation.
 *
 * Implements GGPO-style rollback:
 *   - SaveState()   — snapshot current simulation tick
 *   - LoadState()   — restore to a previous snapshot for re-simulation
 *   - ConfirmInput() — mark a remote tick's input as authoritative
 *   - NeedsRollback() — true when a later confirmed input contradicts a prediction
 *
 * Works with Engine::net::NetContext for transport and Runtime::StateSync
 * for entity-state diffing.
 *
 * Usage:
 * @code
 *   RollbackNetcode rollback;
 *   rollback.Init(60 /*fps*\/);
 *   // each local tick:
 *   rollback.RecordLocalInput(tick, localInput);
 *   if (rollback.NeedsRollback())
 *       rollback.PerformRollback([&](uint64_t t){ SimulateTick(t); });
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Engine::net {

/// Raw input bytes for one peer at one tick.
struct PeerInput {
    uint32_t             peerId{0};
    uint64_t             tick{0};
    std::vector<uint8_t> inputBytes;   ///< Game-defined; serialise your input struct here
    bool                 confirmed{false};
};

/// Lightweight simulation snapshot (game code fills payload via callback).
struct RollbackFrame {
    uint64_t             tick{0};
    std::vector<uint8_t> statePayload; ///< Opaque blob from SaveStateFn
};

using SaveStateFn    = std::function<std::vector<uint8_t>()>;
using LoadStateFn    = std::function<void(const std::vector<uint8_t>&)>;
using SimulateTickFn = std::function<void(uint64_t tick)>;

/// RollbackNetcode — manages input history, state snapshots, and rollback.
class RollbackNetcode {
public:
    /// @param maxRollbackTicks  Maximum ticks we'll roll back (default 8).
    void Init(uint32_t targetFps = 60, uint32_t maxRollbackTicks = 8);

    // ── Callbacks (set before first tick) ─────────────────────────────────
    void SetSaveStateFn(SaveStateFn fn);
    void SetLoadStateFn(LoadStateFn fn);

    // ── Per-tick interface ─────────────────────────────────────────────────
    /// Called by local client before advancing simulation.
    void RecordLocalInput(uint64_t tick, const std::vector<uint8_t>& input);

    /// Called when a remote peer's input for a past tick arrives.
    void ConfirmRemoteInput(const PeerInput& input);

    /// Returns true if confirmed remote inputs differ from predicted inputs.
    bool NeedsRollback() const;

    /// Saves a state snapshot for the current tick (call after simulating).
    void SaveState(uint64_t tick);

    /// Rolls back to the earliest mis-predicted tick and re-simulates
    /// up to currentTick using simulateFn.
    void PerformRollback(uint64_t currentTick, SimulateTickFn simulateFn);

    // ── Diagnostics ────────────────────────────────────────────────────────
    uint64_t CurrentTick()        const;
    uint32_t MaxRollbackTicks()   const;
    uint32_t PendingRollbacks()   const;   ///< How many ticks need re-sim
    void     Reset();

private:
    bool     InputsDiffer(const PeerInput& predicted,
                           const PeerInput& confirmed) const;

    uint32_t m_targetFps{60};
    uint32_t m_maxRollbackTicks{8};
    uint64_t m_currentTick{0};

    SaveStateFn m_saveState;
    LoadStateFn m_loadState;

    /// input[tick][peerId] = PeerInput
    std::unordered_map<uint64_t,
        std::unordered_map<uint32_t, PeerInput>> m_predictedInputs;
    std::unordered_map<uint64_t,
        std::unordered_map<uint32_t, PeerInput>> m_confirmedInputs;

    std::unordered_map<uint64_t, RollbackFrame> m_frames;

    uint64_t m_rollbackToTick{UINT64_MAX}; ///< earliest mis-predicted tick
};

} // namespace Engine::net
