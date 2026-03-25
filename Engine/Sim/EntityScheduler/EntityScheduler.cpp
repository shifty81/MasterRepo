#include "Engine/Sim/EntityScheduler/EntityScheduler.h"
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace Engine {

// ── Internal entry ────────────────────────────────────────────────────────
struct ScheduledEntity {
    EntityTickInfo info;
    TickCallback   callback;
    float          accumulator{0.0f}; ///< Seconds since last tick
};

// ── Impl ──────────────────────────────────────────────────────────────────
struct EntityScheduler::Impl {
    std::unordered_map<EntityId, ScheduledEntity> entries;
    SchedulerStats stats;

    ScheduledEntity* Find(EntityId id) {
        auto it = entries.find(id);
        return it != entries.end() ? &it->second : nullptr;
    }
};

// ── Constructor / Destructor ──────────────────────────────────────────────
EntityScheduler::EntityScheduler()  : m_impl(std::make_unique<Impl>()) {}
EntityScheduler::~EntityScheduler() = default;

// ── Registration ──────────────────────────────────────────────────────────
void EntityScheduler::Register(const EntityTickInfo& info, TickCallback callback) {
    ScheduledEntity se;
    se.info     = info;
    se.callback = std::move(callback);
    m_impl->entries[info.entityId] = std::move(se);
}

void EntityScheduler::Unregister(EntityId id) {
    m_impl->entries.erase(id);
}

bool EntityScheduler::IsRegistered(EntityId id) const {
    return m_impl->entries.count(id) != 0;
}

// ── Runtime control ───────────────────────────────────────────────────────
void EntityScheduler::SetEnabled(EntityId id, bool enabled) {
    if (auto* se = m_impl->Find(id)) se->info.enabled = enabled;
}

void EntityScheduler::SetPriority(EntityId id, TickPriority priority) {
    if (auto* se = m_impl->Find(id)) se->info.priority = priority;
}

void EntityScheduler::SetInterval(EntityId id, float intervalSeconds) {
    if (auto* se = m_impl->Find(id)) se->info.tickInterval = intervalSeconds;
}

// ── Tick ──────────────────────────────────────────────────────────────────
void EntityScheduler::Tick(float dt, double totalBudgetMs) {
    // Advance accumulators
    for (auto& [id, se] : m_impl->entries) {
        if (se.info.enabled) se.accumulator += dt;
    }

    // Collect entities ready to tick, sorted by priority then insertion order
    std::vector<ScheduledEntity*> ready;
    ready.reserve(m_impl->entries.size());
    for (auto& [id, se] : m_impl->entries) {
        if (!se.info.enabled) continue;
        if (se.accumulator >= se.info.tickInterval) {
            ready.push_back(&se);
        }
    }

    std::stable_sort(ready.begin(), ready.end(), [](const ScheduledEntity* a,
                                                     const ScheduledEntity* b) {
        return static_cast<int>(a->info.priority) < static_cast<int>(b->info.priority);
    });

    double budgetUsed = 0.0;
    uint64_t ticked = 0, skipped = 0;

    for (auto* se : ready) {
        bool isCritical = se->info.priority == TickPriority::Critical;
        bool budgetOk   = (budgetUsed < totalBudgetMs) || isCritical;
        if (!budgetOk) {
            ++skipped;
            continue;
        }

        auto t0 = std::chrono::high_resolution_clock::now();
        if (se->callback) se->callback(se->info.entityId, se->accumulator);
        auto t1 = std::chrono::high_resolution_clock::now();

        double elapsed = std::chrono::duration<double, std::milli>(t1 - t0).count();
        budgetUsed += elapsed;
        se->accumulator = 0.0f;
        ++ticked;

        if (elapsed > se->info.maxBudgetMs) ++m_impl->stats.budgetOverruns;
    }

    m_impl->stats.totalTicked  += ticked;
    m_impl->stats.totalSkipped += skipped;
    ++m_impl->stats.frameCount;

    uint64_t frames = m_impl->stats.frameCount;
    if (frames > 0) {
        m_impl->stats.avgTickedPerFrame =
            static_cast<double>(m_impl->stats.totalTicked) / static_cast<double>(frames);
    }
}

// ── Force update ──────────────────────────────────────────────────────────
void EntityScheduler::ForceUpdate(EntityId id, float dt) {
    auto* se = m_impl->Find(id);
    if (!se || !se->callback) return;
    se->callback(id, dt);
    se->accumulator = 0.0f;
}

// ── Queries ───────────────────────────────────────────────────────────────
size_t         EntityScheduler::RegisteredCount() const { return m_impl->entries.size(); }
SchedulerStats EntityScheduler::GetStats()        const { return m_impl->stats; }
void           EntityScheduler::ResetStats()            { m_impl->stats = {}; }

} // namespace Engine
