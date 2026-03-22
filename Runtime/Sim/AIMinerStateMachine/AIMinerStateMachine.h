#pragma once
/**
 * @file AIMinerStateMachine.h
 * @brief Autonomous AI miner state machine for space-sim economies.
 *
 * Adapted from Nova-Forge-Expeditions (atlas::sim → Runtime namespace).
 *
 * Manages multiple AI miners through their full lifecycle:
 *   Idle → SelectTarget → TravelToField → Mining → CargoFull →
 *   ReturnToStation → SellOre → (repeat) | Dead (respawn countdown)
 *
 * MinerConfig: entityId, cargoCapacity, miningYieldPerSec, travelSpeed,
 *              sellPricePerUnit, respawnDelay.
 * MinerRuntime: live state including cargoFill, stateTimer, earnings, cycles.
 *
 * AIMinerStateMachine:
 *   - AddMiner(config) / RemoveMiner(entityId) / KillMiner(entityId)
 *   - SetAvailableDeposits(ids): list of deposit entity ids to mine
 *   - Tick(dt): advance all miners
 *   - GetMiner(id): read-only runtime state
 *   - CountInState(state) / TotalEarnings() / TotalCycles()
 *   - SetFieldDistance(d): travel time = distance / travelSpeed
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

// ── Miner states ──────────────────────────────────────────────────────────
enum class MinerState : uint8_t {
    Idle, SelectTarget, TravelToField, Mining, CargoFull,
    ReturnToStation, SellOre, Dead
};

// ── Config ────────────────────────────────────────────────────────────────
struct MinerConfig {
    uint32_t entityId            = 0;
    float    cargoCapacity       = 400.0f;  ///< m³
    float    miningYieldPerSec   = 2.0f;    ///< m³/s
    float    travelSpeed         = 100.0f;  ///< units/s
    float    sellPricePerUnit    = 10.0f;   ///< credits/m³
    float    respawnDelay        = 60.0f;   ///< seconds
};

// ── Runtime state ─────────────────────────────────────────────────────────
struct MinerRuntime {
    MinerConfig config;
    MinerState  state            = MinerState::Idle;
    float       cargoFill        = 0.0f;
    float       stateTimer       = 0.0f;
    float       totalEarnings    = 0.0f;
    float       travelProgress   = 0.0f;  ///< 0..1
    uint32_t    targetDepositId  = 0;
    uint32_t    cyclesCompleted  = 0;
    float       respawnTimer     = 0.0f;
};

// ── State machine ─────────────────────────────────────────────────────────
class AIMinerStateMachine {
public:
    AIMinerStateMachine() = default;

    bool AddMiner(const MinerConfig& config);
    bool RemoveMiner(uint32_t entityId);
    bool KillMiner(uint32_t entityId);

    void SetAvailableDeposits(const std::vector<uint32_t>& depositIds);

    void Tick(float deltaSeconds);

    const MinerRuntime* GetMiner(uint32_t entityId) const;
    size_t              MinerCount()  const { return m_miners.size(); }
    size_t              CountInState(MinerState state) const;
    float               TotalEarnings() const;
    uint32_t            TotalCycles()   const;

    void  SetFieldDistance(float d) { m_fieldDistance = d; }
    float FieldDistance()      const { return m_fieldDistance; }

private:
    void     tickMiner(MinerRuntime& m, float dt);
    uint32_t pickDeposit() const;

    std::vector<MinerRuntime> m_miners;
    std::vector<uint32_t>     m_availableDeposits;
    float                     m_fieldDistance{500.0f};
};

} // namespace Runtime
