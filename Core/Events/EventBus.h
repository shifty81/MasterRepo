#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <algorithm>

namespace atlas {

/**
 * Lightweight event payload.
 *
 * Events carry a type tag (string), an optional integer parameter, and an
 * optional floating-point parameter.  This deliberately avoids heap
 * allocation and virtual dispatch so that thousands of events per tick are
 * cheap.
 */
struct Event {
    std::string type;
    uint32_t    senderId  = 0;
    int64_t     intParam  = 0;
    double      floatParam = 0.0;
    std::string strParam;
};

/**
 * Subscription handle returned by EventBus::Subscribe.
 * Pass to EventBus::Unsubscribe to remove the listener.
 */
using SubscriptionId = uint64_t;

/**
 * EventBus — publish / subscribe event dispatch for the Atlas Engine.
 *
 * Design goals
 *   - Type-safe callbacks via std::function
 *   - Per-event-type subscription lists for O(1) dispatch lookup
 *   - Thread-safe subscribe / unsubscribe / publish
 *   - Deferred (queued) event delivery for deterministic tick ordering
 *   - Wildcard ("*") subscriptions receive every event
 *
 * Typical usage:
 * @code
 *   atlas::EventBus bus;
 *
 *   auto id = bus.Subscribe("DAMAGE", [](const atlas::Event& e) {
 *       std::cout << "Entity " << e.senderId << " took "
 *                 << e.floatParam << " damage\n";
 *   });
 *
 *   bus.Publish({"DAMAGE", entityId, 0, 42.5});
 *   // Listener fires immediately.
 *
 *   bus.Enqueue({"SHIELD_DOWN", entityId});
 *   bus.Flush();   // Deferred events dispatched here.
 *
 *   bus.Unsubscribe(id);
 * @endcode
 */
class EventBus {
public:
    using Callback = std::function<void(const Event&)>;

    EventBus() = default;

    // ── Subscribe / Unsubscribe ───────────────────────────────────

    /**
     * Register a callback for events with the given @p eventType.
     * Use "*" to receive all events (wildcard).
     * Returns a handle that can be passed to Unsubscribe().
     */
    SubscriptionId Subscribe(const std::string& eventType, Callback cb);

    /**
     * Remove a previously registered subscription.
     * Safe to call with an invalid or already-removed id (no-op).
     */
    void Unsubscribe(SubscriptionId id);

    // ── Publish / Enqueue ─────────────────────────────────────────

    /**
     * Synchronously dispatch @p event to all matching listeners *now*.
     */
    void Publish(const Event& event);

    /**
     * Queue @p event for deferred delivery.
     * Call Flush() to dispatch all queued events.
     */
    void Enqueue(const Event& event);

    /**
     * Dispatch all queued events in FIFO order, then clear the queue.
     */
    void Flush();

    // ── Queries ───────────────────────────────────────────────────

    /** Number of active subscriptions (across all event types). */
    size_t SubscriptionCount() const;

    /** Number of events waiting in the deferred queue. */
    size_t QueueSize() const;

    /** Number of events published so far (both sync and deferred). */
    uint64_t TotalPublished() const { return m_totalPublished; }

    /** Remove all subscriptions and clear the queue. */
    void Reset();

private:
    struct Subscription {
        SubscriptionId id = 0;
        std::string    eventType;
        Callback       callback;
    };

    mutable std::mutex m_mutex;

    // Per-type subscription lists for fast dispatch.
    std::unordered_map<std::string, std::vector<Subscription>> m_subs;

    // Wildcard subscribers (event type "*").
    std::vector<Subscription> m_wildcardSubs;

    // Deferred event queue.
    std::vector<Event> m_queue;

    SubscriptionId m_nextId = 1;
    uint64_t       m_totalPublished = 0;
};

} // namespace atlas
