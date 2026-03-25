#pragma once
/**
 * @file EntityScheduler.h
 * @brief Per-entity tick scheduling with priority lanes, throttle, and burst-budget control.
 *
 * EntityScheduler decouples the game's fixed simulation step from per-entity
 * update costs.  Entities are grouped into priority lanes and can be throttled
 * so that expensive entities are updated less frequently:
 *
 *   TickPriority: Critical, High, Normal, Low, Background.
 *
 *   EntityTickInfo:
 *     - entityId, priority, tickInterval (seconds between ticks),
 *       maxBudgetMs (wall-time budget per tick), enabled flag.
 *
 *   EntityScheduler:
 *     - Register(info, callback): subscribe an entity for scheduled ticks.
 *     - Unregister(id).
 *     - SetEnabled(id, enabled).
 *     - SetPriority(id, priority).
 *     - SetInterval(id, seconds).
 *     - Tick(dt, totalBudgetMs): advance the scheduler by `dt` seconds.
 *         Entities whose accumulated time exceeds their interval are ticked in
 *         priority order; ticking stops when `totalBudgetMs` is exhausted.
 *     - ForceUpdate(id): tick an entity immediately regardless of interval.
 *     - Stats(): ticked count, skipped count, budget overruns.
 */

#include <cstdint>
#include <functional>
#include <memory>

namespace Engine {

// ── Entity handle ─────────────────────────────────────────────────────────
using EntityId = uint64_t;
static constexpr EntityId kInvalidEntity = 0;

// ── Tick priority ─────────────────────────────────────────────────────────
enum class TickPriority : uint8_t {
    Critical   = 0,  ///< Always ticked (e.g. player, physics)
    High       = 1,
    Normal     = 2,
    Low        = 3,
    Background = 4,  ///< Only ticked when budget allows
};

// ── Tick registration info ─────────────────────────────────────────────────
struct EntityTickInfo {
    EntityId     entityId{kInvalidEntity};
    TickPriority priority{TickPriority::Normal};
    float        tickInterval{0.0f};   ///< Seconds between updates (0 = every frame)
    float        maxBudgetMs{2.0f};    ///< Max wall-time allowed per tick (ms)
    bool         enabled{true};
};

// ── Tick callback ─────────────────────────────────────────────────────────
/// Called with (entityId, dt) when an entity is ticked.
using TickCallback = std::function<void(EntityId, float dt)>;

// ── Scheduler stats ───────────────────────────────────────────────────────
struct SchedulerStats {
    uint64_t totalTicked{0};
    uint64_t totalSkipped{0};     ///< Skipped because budget was exhausted
    uint64_t budgetOverruns{0};   ///< Entities that exceeded maxBudgetMs
    uint64_t frameCount{0};
    double   avgTickedPerFrame{0.0};
};

// ── EntityScheduler ───────────────────────────────────────────────────────
class EntityScheduler {
public:
    EntityScheduler();
    ~EntityScheduler();

    EntityScheduler(const EntityScheduler&)            = delete;
    EntityScheduler& operator=(const EntityScheduler&) = delete;

    // ── registration ──────────────────────────────────────────
    void Register(const EntityTickInfo& info, TickCallback callback);
    void Unregister(EntityId id);
    bool IsRegistered(EntityId id) const;

    // ── runtime control ───────────────────────────────────────
    void SetEnabled(EntityId id, bool enabled);
    void SetPriority(EntityId id, TickPriority priority);
    void SetInterval(EntityId id, float intervalSeconds);

    // ── scheduling ────────────────────────────────────────────
    /**
     * Advance all registered entities by `dt` seconds.
     * @param dt            Elapsed seconds since last Tick().
     * @param totalBudgetMs Total wall-time budget in milliseconds.
     *                      Critical entities are never skipped.
     *                      Lower-priority entities are skipped when budget runs out.
     */
    void Tick(float dt, double totalBudgetMs = 16.0);

    /// Force an immediate tick for `id`, bypassing interval and budget.
    void ForceUpdate(EntityId id, float dt = 0.0f);

    // ── queries ───────────────────────────────────────────────
    size_t          RegisteredCount() const;
    SchedulerStats  GetStats()        const;
    void            ResetStats();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Engine
