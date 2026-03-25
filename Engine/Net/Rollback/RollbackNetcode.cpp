#include "Engine/Net/Rollback/RollbackNetcode.h"
#include <algorithm>

namespace Engine::net {

void RollbackNetcode::Init(uint32_t targetFps, uint32_t maxRollbackTicks) {
    m_targetFps        = targetFps;
    m_maxRollbackTicks = maxRollbackTicks;
    m_currentTick      = 0;
    m_rollbackToTick   = UINT64_MAX;
}

void RollbackNetcode::SetSaveStateFn(SaveStateFn fn) { m_saveState = std::move(fn); }
void RollbackNetcode::SetLoadStateFn(LoadStateFn fn) { m_loadState = std::move(fn); }

void RollbackNetcode::RecordLocalInput(uint64_t tick,
                                        const std::vector<uint8_t>& input) {
    m_currentTick = tick;
    PeerInput pi;
    pi.peerId     = 0; // local peer is always 0
    pi.tick       = tick;
    pi.inputBytes = input;
    pi.confirmed  = false;
    m_predictedInputs[tick][0] = pi;
}

void RollbackNetcode::ConfirmRemoteInput(const PeerInput& input) {
    PeerInput confirmed = input;
    confirmed.confirmed = true;
    m_confirmedInputs[input.tick][input.peerId] = confirmed;

    // Check whether this confirmation contradicts our prediction
    auto pit = m_predictedInputs.find(input.tick);
    if (pit != m_predictedInputs.end()) {
        auto pit2 = pit->second.find(input.peerId);
        if (pit2 != pit->second.end()) {
            if (InputsDiffer(pit2->second, confirmed)) {
                if (input.tick < m_rollbackToTick)
                    m_rollbackToTick = input.tick;
            }
        }
    }
}

bool RollbackNetcode::NeedsRollback() const {
    return m_rollbackToTick != UINT64_MAX
        && m_rollbackToTick <= m_currentTick;
}

void RollbackNetcode::SaveState(uint64_t tick) {
    if (!m_saveState) return;
    RollbackFrame frame;
    frame.tick         = tick;
    frame.statePayload = m_saveState();
    m_frames[tick]     = std::move(frame);

    // Prune frames older than maxRollbackTicks
    if (tick >= m_maxRollbackTicks) {
        uint64_t oldest = tick - m_maxRollbackTicks;
        for (auto it = m_frames.begin(); it != m_frames.end(); ) {
            if (it->first < oldest)
                it = m_frames.erase(it);
            else
                ++it;
        }
    }
}

void RollbackNetcode::PerformRollback(uint64_t currentTick,
                                       SimulateTickFn simulateFn) {
    if (!NeedsRollback()) return;
    if (!m_loadState || !simulateFn) { m_rollbackToTick = UINT64_MAX; return; }

    // Find the last clean frame at or before the rollback point
    uint64_t restoreTick = m_rollbackToTick;
    while (restoreTick > 0 && m_frames.find(restoreTick) == m_frames.end())
        --restoreTick;

    auto fit = m_frames.find(restoreTick);
    if (fit == m_frames.end()) { m_rollbackToTick = UINT64_MAX; return; }

    m_loadState(fit->second.statePayload);

    // Re-simulate from restoreTick up to currentTick
    for (uint64_t t = restoreTick + 1; t <= currentTick; ++t) {
        simulateFn(t);
        SaveState(t);
    }

    m_rollbackToTick = UINT64_MAX;
}

uint64_t RollbackNetcode::CurrentTick()       const { return m_currentTick; }
uint32_t RollbackNetcode::MaxRollbackTicks()  const { return m_maxRollbackTicks; }

uint32_t RollbackNetcode::PendingRollbacks()  const {
    if (!NeedsRollback()) return 0;
    return (uint32_t)(m_currentTick - m_rollbackToTick + 1);
}

void RollbackNetcode::Reset() {
    m_currentTick    = 0;
    m_rollbackToTick = UINT64_MAX;
    m_predictedInputs.clear();
    m_confirmedInputs.clear();
    m_frames.clear();
}

bool RollbackNetcode::InputsDiffer(const PeerInput& a, const PeerInput& b) const {
    return a.inputBytes != b.inputBytes;
}

} // namespace Engine::net
