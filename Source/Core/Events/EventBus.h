#pragma once
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace NF {

/// @brief Lightweight, synchronous publish-subscribe event bus.
///
/// Events are plain structs/classes. Subscriptions persist until the bus is
/// destroyed or Clear() is called.  Publish() calls all registered handlers
/// inline on the calling thread.
class EventBus {
public:
    /// @brief Register a callback to be invoked whenever an event of type T is published.
    /// @tparam T   The event type (must be copyable).
    /// @param handler Callback with signature void(const T&).
    template<typename T>
    void Subscribe(std::function<void(const T&)> handler) {
        auto key = std::type_index(typeid(T));
        m_Handlers[key].emplace_back(
            [h = std::move(handler)](const void* evt) {
                h(*static_cast<const T*>(evt));
            });
    }

    /// @brief Broadcast an event to all subscribers registered for type T.
    /// @tparam T    The event type.
    /// @param event The event instance; passed by const-ref to each handler.
    template<typename T>
    void Publish(const T& event) {
        auto it = m_Handlers.find(std::type_index(typeid(T)));
        if (it == m_Handlers.end())
            return;
        for (auto& fn : it->second)
            fn(static_cast<const void*>(&event));
    }

    /// @brief Unsubscribe all handlers for every event type.
    void Clear() noexcept { m_Handlers.clear(); }

private:
    using ErasedHandler = std::function<void(const void*)>;
    std::unordered_map<std::type_index, std::vector<ErasedHandler>> m_Handlers;
};

} // namespace NF
