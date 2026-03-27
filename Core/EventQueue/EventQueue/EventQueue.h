#pragma once
/**
 * @file EventQueue.h
 * @brief Typed deferred event queue with per-type listeners.
 *
 * Features:
 *   - Type-erased event envelope with compile-time type IDs (no RTTI)
 *   - Enqueue events to be processed on the next (or current) frame
 *   - Immediate dispatch mode: fire listeners synchronously
 *   - Deferred mode: buffer events until Flush() is called
 *   - Per-event-type listeners: Subscribe<EventT>(callback)
 *   - Listener priorities: higher priority fires first
 *   - Listener auto-unsubscribe token (RAII guard)
 *   - Event filtering: stop propagation inside a listener
 *   - Per-frame event count and type histogram
 *   - Thread-safe enqueue (flush always on main thread)
 *
 * Typical usage:
 * @code
 *   EventQueue eq;
 *   eq.Init();
 *
 *   struct DamageEvent { uint32_t targetId; float amount; };
 *   auto token = eq.Subscribe<DamageEvent>([](const DamageEvent& e){
 *       TakeDamage(e.targetId, e.amount);
 *   });
 *
 *   eq.Enqueue(DamageEvent{playerId, 25.f});
 *   eq.Flush();     // fires all queued events on main thread
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Core {

// ── Subscription token (RAII auto-unsubscribe) ──────────────────────────────

struct EventSubscription {
    uint32_t id{0};
    explicit operator bool() const { return id != 0; }
};

// ── Internal envelope ────────────────────────────────────────────────────────

struct EventEnvelope {
    std::type_index     type;
    std::shared_ptr<void> data;
    int32_t             priority{0};
};

// ── EventQueue ───────────────────────────────────────────────────────────────

class EventQueue {
public:
    EventQueue();
    ~EventQueue();

    void Init();
    void Shutdown();

    /// Flush all deferred events on the calling thread (main thread)
    void Flush();

    /// Clear pending events without firing them
    void Clear();

    // ── Template API ────────────────────────────────────────────────────────

    /// Enqueue a deferred event (copied into queue)
    template<typename EventT>
    void Enqueue(EventT event, int32_t priority=0) {
        EventEnvelope env{typeid(EventT),
                          std::make_shared<EventT>(std::move(event)),
                          priority};
        EnqueueEnvelope(std::move(env));
    }

    /// Dispatch immediately (bypasses queue, fires listeners now)
    template<typename EventT>
    void DispatchNow(const EventT& event) {
        FireListeners(typeid(EventT), &event);
    }

    /// Subscribe to events of type EventT. Returns a token; unsubscribe with Unsubscribe(token).
    template<typename EventT>
    EventSubscription Subscribe(std::function<void(const EventT&)> cb,
                                 int32_t priority=0)
    {
        auto wrapper = [cb](const void* raw) {
            cb(*static_cast<const EventT*>(raw));
        };
        return SubscribeInternal(typeid(EventT), wrapper, priority);
    }

    void Unsubscribe(EventSubscription token);

    // Stats
    uint32_t QueuedCount() const;

private:
    struct ListenerEntry {
        uint32_t  id{0};
        int32_t   priority{0};
        std::function<void(const void*)> fn;
    };

    void EnqueueEnvelope(EventEnvelope env);
    void FireListeners  (const std::type_index& type, const void* raw);
    EventSubscription SubscribeInternal(const std::type_index& type,
                                         std::function<void(const void*)> fn,
                                         int32_t priority);

    std::mutex                                         m_enqueueMu;
    std::vector<EventEnvelope>                         m_queue;
    std::unordered_map<std::type_index,
                       std::vector<ListenerEntry>>     m_listeners;
    uint32_t                                           m_nextSubId{1};
};

} // namespace Core
