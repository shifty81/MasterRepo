#pragma once

#include "Core/Events/Event.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace Core {

/// Opaque handle returned by Subscribe(); pass to Unsubscribe() to remove a handler.
using HandlerID = uint64_t;

/// Central publish/subscribe event bus.
///
/// All subscriptions are keyed by EventTypeID (derived from the compile-time
/// string hash).  Publishing an event invokes every handler registered for
/// that type, in subscription order.
///
/// Thread safety: Subscribe, Unsubscribe, and Publish are all mutex-guarded
/// so the bus can be shared across threads.
///
/// Usage:
///   Core::EventBus bus;
///
///   auto id = bus.Subscribe<MyEvent>([](MyEvent& e) {
///       // handle event
///   });
///
///   MyEvent evt;
///   bus.Publish(evt);       // invokes the lambda above
///
///   bus.Unsubscribe(id);    // remove the handler
class EventBus {
public:
    EventBus()  = default;
    ~EventBus() = default;

    EventBus(const EventBus&)            = delete;
    EventBus& operator=(const EventBus&) = delete;

    /// Subscribe a handler for event type T.
    /// Returns a unique HandlerID that can be used to unsubscribe later.
    template <typename T>
    HandlerID Subscribe(std::function<void(T&)> handler) {
        // Wrap the typed handler in a type-erased std::function<void(Event&)>.
        auto wrapper = [fn = std::move(handler)](Event& e) {
            fn(static_cast<T&>(e));
        };
        return SubscribeByType(T::StaticType(), std::move(wrapper));
    }

    /// Remove a previously registered handler.
    void Unsubscribe(HandlerID id);

    /// Publish an event to all subscribers of its runtime type.
    void Publish(Event& event);

private:
    /// Internal record kept per handler.
    struct HandlerRecord {
        HandlerID                    ID;
        std::function<void(Event&)>  Callback;
    };

    /// Register a type-erased handler for a given EventTypeID.
    HandlerID SubscribeByType(EventTypeID typeID,
                              std::function<void(Event&)> callback);

    std::mutex                                                m_Mutex;
    HandlerID                                                 m_NextID = 1;
    std::unordered_map<EventTypeID, std::vector<HandlerRecord>> m_Handlers;
    std::unordered_map<HandlerID, EventTypeID>                m_HandlerTypeMap;
};

} // namespace Core
