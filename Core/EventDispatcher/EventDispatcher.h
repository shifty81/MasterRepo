#pragma once
/**
 * @file EventDispatcher.h
 * @brief Typed broadcast event dispatcher with priority listeners.
 *
 * EventDispatcher is a global-or-local typed pub/sub system that is separate
 * from Core::Events::EventBus (which uses a type-erased approach).
 * This implementation uses std::any-typed payloads keyed by event-name string,
 * giving a lightweight scripting-friendly API while retaining type safety:
 *
 *   - Subscribe<T>(event, listener, priority, oneShot)
 *   - Dispatch<T>(event, payload)     — synchronous, sorted by priority
 *   - Defer<T>(event, payload)        — queued; FlushDeferred() processes all
 *   - Unsubscribe by returned ListenerID
 *
 * Priorities: higher numeric value = called first.
 * One-shot listeners are removed immediately after first invocation.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <any>
#include <functional>
#include <optional>

namespace Core {

// ── Listener handle ───────────────────────────────────────────────────────
using ListenerID = uint64_t;
constexpr ListenerID kInvalidListener = 0;

// ── Type-erased listener ──────────────────────────────────────────────────
struct EventListener {
    ListenerID                      id{kInvalidListener};
    std::string                     event;
    int32_t                         priority{0};
    bool                            oneShot{false};
    std::function<void(const std::any&)> fn;
};

// ── Deferred event ────────────────────────────────────────────────────────
struct DeferredEvent {
    std::string event;
    std::any    payload;
};

// ── Dispatcher ───────────────────────────────────────────────────────────
class EventDispatcher {
public:
    EventDispatcher();
    ~EventDispatcher();

    // ── subscribe ─────────────────────────────────────────────

    /// Subscribe a typed listener.
    template<typename T>
    ListenerID Subscribe(const std::string& event,
                         std::function<void(const T&)> fn,
                         int32_t priority = 0,
                         bool oneShot = false)
    {
        EventListener listener;
        listener.id       = nextId();
        listener.event    = event;
        listener.priority = priority;
        listener.oneShot  = oneShot;
        listener.fn       = [fn = std::move(fn)](const std::any& a) {
            if (auto* p = std::any_cast<T>(&a)) fn(*p);
        };
        addListener(std::move(listener));
        return listener.id;
    }

    /// Subscribe an untyped (raw std::any) listener.
    ListenerID SubscribeRaw(const std::string& event,
                             std::function<void(const std::any&)> fn,
                             int32_t priority = 0,
                             bool oneShot = false);

    void Unsubscribe(ListenerID id);
    void UnsubscribeAll(const std::string& event);

    bool HasListeners(const std::string& event) const;
    size_t ListenerCount(const std::string& event = "") const;

    // ── dispatch (synchronous) ────────────────────────────────

    template<typename T>
    void Dispatch(const std::string& event, const T& payload) {
        dispatchRaw(event, std::any(payload));
    }

    void DispatchRaw(const std::string& event, const std::any& payload) {
        dispatchRaw(event, payload);
    }

    // ── deferred queue ────────────────────────────────────────

    template<typename T>
    void Defer(const std::string& event, const T& payload) {
        deferRaw(event, std::any(payload));
    }

    void DeferRaw(const std::string& event, const std::any& payload) {
        deferRaw(event, payload);
    }

    /// Process all queued deferred events in FIFO order.
    void FlushDeferred();
    size_t DeferredCount() const;

    // ── maintenance ───────────────────────────────────────────
    void Clear();  ///< Remove all listeners and deferred events.

    // ── event enumeration ─────────────────────────────────────
    std::vector<std::string> RegisteredEvents() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};

    ListenerID nextId();
    void addListener(EventListener listener);
    void dispatchRaw(const std::string& event, const std::any& payload);
    void deferRaw(const std::string& event, const std::any& payload);
};

} // namespace Core
