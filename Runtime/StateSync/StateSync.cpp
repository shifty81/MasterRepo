#include "Runtime/StateSync/StateSync.h"
#include <algorithm>
#include <deque>

namespace Runtime {

// ── Impl ─────────────────────────────────────────────────────────────────────
struct StateSync::Impl {
    uint32_t                  rollbackWindow{60};
    uint32_t                  desyncTolerance{0};
    std::deque<WorldSnapshot> local;     // ring of captured snapshots
    uint64_t                  currentTick{0};
    uint32_t                  desyncCount{0};
    bool                      inSync{true};
    std::vector<SnapshotCallback> onSnapshot;
    std::vector<ConflictCallback> onDesync;
};

StateSync::StateSync() : m_impl(new Impl()) {}
StateSync::~StateSync() { delete m_impl; }

void StateSync::SetRollbackWindow(uint32_t ticks) {
    m_impl->rollbackWindow = ticks;
}
void StateSync::SetDesyncTolerance(uint32_t bytes) {
    m_impl->desyncTolerance = bytes;
}

void StateSync::Capture(const WorldSnapshot& snap) {
    m_impl->currentTick = snap.tick;
    m_impl->local.push_back(snap);
    while (m_impl->local.size() > m_impl->rollbackWindow)
        m_impl->local.pop_front();
    for (auto& cb : m_impl->onSnapshot) cb(snap);
}

void StateSync::ApplyRemote(const WorldSnapshot& remote) {
    // Find local snapshot at the same tick.
    auto it = std::find_if(m_impl->local.begin(), m_impl->local.end(),
        [&](const WorldSnapshot& s){ return s.tick == remote.tick; });
    if (it == m_impl->local.end()) return;

    // Compare entity states.
    for (auto& re : remote.entities) {
        auto le = std::find_if(it->entities.begin(), it->entities.end(),
            [&](const EntityState& e){
                return e.entityId == re.entityId &&
                       e.componentType == re.componentType;
            });
        if (le == it->entities.end()) continue;
        // Byte diff exceeds tolerance → desync.
        size_t diff = 0;
        size_t common = std::min(le->payload.size(), re.payload.size());
        for (size_t i = 0; i < common; ++i)
            if (le->payload[i] != re.payload[i]) ++diff;
        diff += std::max(le->payload.size(), re.payload.size()) - common;
        if (diff > m_impl->desyncTolerance) {
            ++m_impl->desyncCount;
            m_impl->inSync = false;
            for (auto& cb : m_impl->onDesync) cb(*le, re);
        }
    }
}

bool StateSync::Rollback(uint64_t toTick) {
    auto it = std::find_if(m_impl->local.rbegin(), m_impl->local.rend(),
        [&](const WorldSnapshot& s){ return s.tick <= toTick; });
    if (it == m_impl->local.rend()) return false;
    m_impl->currentTick = it->tick;
    m_impl->inSync      = true;
    // Truncate history after rollback point.
    auto forward = std::find_if(m_impl->local.begin(), m_impl->local.end(),
        [&](const WorldSnapshot& s){ return s.tick > toTick; });
    m_impl->local.erase(forward, m_impl->local.end());
    return true;
}

uint64_t      StateSync::CurrentTick()  const { return m_impl->currentTick; }
bool          StateSync::IsInSync()     const { return m_impl->inSync; }
uint32_t      StateSync::DesyncCount()  const { return m_impl->desyncCount; }

WorldSnapshot StateSync::GetSnapshot(uint64_t tick) const {
    for (auto& s : m_impl->local) if (s.tick == tick) return s;
    return {};
}

void StateSync::OnSnapshot(SnapshotCallback cb) {
    m_impl->onSnapshot.push_back(std::move(cb));
}
void StateSync::OnDesync(ConflictCallback cb) {
    m_impl->onDesync.push_back(std::move(cb));
}

} // namespace Runtime
