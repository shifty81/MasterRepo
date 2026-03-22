#include "Runtime/Sim/AIMinerStateMachine/AIMinerStateMachine.h"
#include <algorithm>

namespace Runtime {

bool AIMinerStateMachine::AddMiner(const MinerConfig& config) {
    for (auto& m : m_miners) if (m.config.entityId == config.entityId) return false;
    MinerRuntime rt; rt.config = config; rt.state = MinerState::Idle;
    m_miners.push_back(rt);
    return true;
}

bool AIMinerStateMachine::RemoveMiner(uint32_t entityId) {
    auto it = std::remove_if(m_miners.begin(), m_miners.end(),
        [entityId](const MinerRuntime& m){ return m.config.entityId == entityId; });
    if (it == m_miners.end()) return false;
    m_miners.erase(it, m_miners.end()); return true;
}

bool AIMinerStateMachine::KillMiner(uint32_t entityId) {
    for (auto& m : m_miners) {
        if (m.config.entityId == entityId && m.state != MinerState::Dead) {
            m.state = MinerState::Dead;
            m.stateTimer = 0; m.respawnTimer = m.config.respawnDelay; m.cargoFill = 0;
            return true;
        }
    }
    return false;
}

void AIMinerStateMachine::SetAvailableDeposits(const std::vector<uint32_t>& ids) {
    m_availableDeposits = ids;
}

void AIMinerStateMachine::Tick(float dt) {
    for (auto& m : m_miners) tickMiner(m, dt);
}

const MinerRuntime* AIMinerStateMachine::GetMiner(uint32_t entityId) const {
    for (auto& m : m_miners) if (m.config.entityId == entityId) return &m;
    return nullptr;
}

size_t AIMinerStateMachine::CountInState(MinerState state) const {
    size_t n = 0;
    for (auto& m : m_miners) if (m.state == state) ++n;
    return n;
}

float AIMinerStateMachine::TotalEarnings() const {
    float s = 0; for (auto& m : m_miners) s += m.totalEarnings; return s;
}

uint32_t AIMinerStateMachine::TotalCycles() const {
    uint32_t s = 0; for (auto& m : m_miners) s += m.cyclesCompleted; return s;
}

void AIMinerStateMachine::tickMiner(MinerRuntime& m, float dt) {
    m.stateTimer += dt;
    switch (m.state) {
    case MinerState::Idle:
        m.state = MinerState::SelectTarget; m.stateTimer = 0; break;

    case MinerState::SelectTarget: {
        uint32_t dep = pickDeposit();
        if (!dep) { m.state = MinerState::Idle; m.stateTimer = 0; break; }
        m.targetDepositId = dep; m.state = MinerState::TravelToField;
        m.stateTimer = 0; m.travelProgress = 0; break;
    }
    case MinerState::TravelToField: {
        float tt = m_fieldDistance / m.config.travelSpeed;
        m.travelProgress += dt / tt;
        if (m.travelProgress >= 1.0f) {
            m.travelProgress = 1.0f; m.state = MinerState::Mining; m.stateTimer = 0;
        }
        break;
    }
    case MinerState::Mining:
        m.cargoFill += m.config.miningYieldPerSec * dt;
        if (m.cargoFill >= m.config.cargoCapacity) {
            m.cargoFill = m.config.cargoCapacity; m.state = MinerState::CargoFull; m.stateTimer = 0;
        }
        break;

    case MinerState::CargoFull:
        m.state = MinerState::ReturnToStation; m.stateTimer = 0; m.travelProgress = 0; break;

    case MinerState::ReturnToStation: {
        float tt = m_fieldDistance / m.config.travelSpeed;
        m.travelProgress += dt / tt;
        if (m.travelProgress >= 1.0f) {
            m.travelProgress = 1.0f; m.state = MinerState::SellOre; m.stateTimer = 0;
        }
        break;
    }
    case MinerState::SellOre:
        m.totalEarnings += m.cargoFill * m.config.sellPricePerUnit;
        m.cargoFill = 0; ++m.cyclesCompleted;
        m.state = MinerState::Idle; m.stateTimer = 0; break;

    case MinerState::Dead:
        m.respawnTimer -= dt;
        if (m.respawnTimer <= 0) { m.respawnTimer = 0; m.state = MinerState::Idle; m.stateTimer = 0; }
        break;
    }
}

uint32_t AIMinerStateMachine::pickDeposit() const {
    if (m_availableDeposits.empty()) return 0;
    return m_availableDeposits[0];
}

} // namespace Runtime
