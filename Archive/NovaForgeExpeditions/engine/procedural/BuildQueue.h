#pragma once
#include <cstdint>
#include <vector>

namespace atlas::procedural {

/// The kind of build action.
enum class BuildOrderType : uint8_t {
    Construct,
    Upgrade,
    Repair,
    Dismantle
};

/// A single build/upgrade/repair order.
struct BuildOrder {
    uint32_t id = 0;
    BuildOrderType type = BuildOrderType::Construct;
    uint8_t moduleType = 0;
    uint32_t targetSlot = 0;
    uint8_t targetTier = 1;
    float totalTimeSeconds = 0.0f;
    float elapsedSeconds = 0.0f;
    bool paused = false;
    uint8_t priority = 128;

    /// Progress as a fraction in [0, 1].
    float Progress() const;

    /// True when the order has completed.
    bool IsComplete() const { return elapsedSeconds >= totalTimeSeconds; }

    /// Seconds remaining until completion.
    float RemainingSeconds() const;
};

/// Manages an ordered queue of build operations, ticking them forward
/// over time and collecting completed orders.
class BuildQueue {
public:
    /// Enqueue a build order; returns the assigned order id.
    uint32_t AddOrder(BuildOrder order);

    /// Cancel (remove) an active order by id.
    void RemoveOrder(uint32_t orderId);

    /// Pause an active order.
    void PauseOrder(uint32_t orderId);

    /// Resume a paused order.
    void ResumeOrder(uint32_t orderId);

    /// Advance all non-paused orders by deltaSeconds.
    void Tick(float deltaSeconds);

    /// Look up an active order by id (nullptr if not found).
    const BuildOrder* GetOrder(uint32_t orderId) const;

    /// Number of active (pending) orders.
    size_t QueueSize() const { return m_active.size(); }

    /// Number of completed orders.
    size_t CompletedCount() const { return m_completed.size(); }

    /// Get the list of completed orders.
    const std::vector<BuildOrder>& GetCompletedOrders() const { return m_completed; }

    /// Discard all completed orders.
    void ClearCompleted() { m_completed.clear(); }

    /// True when there are no active orders.
    bool IsEmpty() const { return m_active.empty(); }

    /// Return active orders sorted by priority (highest first).
    std::vector<BuildOrder> GetOrdersByPriority() const;

    /// Sum of remaining time across all active orders.
    float TotalRemainingTime() const;

private:
    std::vector<BuildOrder> m_active;
    std::vector<BuildOrder> m_completed;
    uint32_t m_nextId = 1;
};

} // namespace atlas::procedural
