#include "Core/EventDispatcher/EventDispatcher.h"
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <atomic>

namespace Core {

struct EventDispatcher::Impl {
    std::unordered_map<std::string, std::vector<EventListener>> listeners;
    std::vector<DeferredEvent>                                   deferred;
    std::atomic<uint64_t>                                        nextId{1};

    // Returns sorted (descending priority) listeners for an event.
    std::vector<EventListener>* Get(const std::string& event) {
        auto it = listeners.find(event);
        return it != listeners.end() ? &it->second : nullptr;
    }
    void Sort(std::vector<EventListener>& vec) {
        std::stable_sort(vec.begin(), vec.end(),
            [](const EventListener& a, const EventListener& b){
                return a.priority > b.priority;
            });
    }
};

EventDispatcher::EventDispatcher() : m_impl(new Impl()) {}
EventDispatcher::~EventDispatcher() { delete m_impl; }

ListenerID EventDispatcher::nextId() {
    return m_impl->nextId.fetch_add(1, std::memory_order_relaxed);
}

void EventDispatcher::addListener(EventListener listener) {
    auto& vec = m_impl->listeners[listener.event];
    vec.push_back(std::move(listener));
    m_impl->Sort(vec);
}

ListenerID EventDispatcher::SubscribeRaw(
    const std::string& event,
    std::function<void(const std::any&)> fn,
    int32_t priority,
    bool oneShot)
{
    EventListener listener;
    listener.id       = nextId();
    listener.event    = event;
    listener.priority = priority;
    listener.oneShot  = oneShot;
    listener.fn       = std::move(fn);
    addListener(std::move(listener));
    return listener.id;
}

void EventDispatcher::Unsubscribe(ListenerID id) {
    for (auto& [event, vec] : m_impl->listeners) {
        auto it = std::remove_if(vec.begin(), vec.end(),
            [id](const EventListener& l){ return l.id == id; });
        vec.erase(it, vec.end());
    }
}

void EventDispatcher::UnsubscribeAll(const std::string& event) {
    m_impl->listeners.erase(event);
}

bool EventDispatcher::HasListeners(const std::string& event) const {
    auto it = m_impl->listeners.find(event);
    return it != m_impl->listeners.end() && !it->second.empty();
}

size_t EventDispatcher::ListenerCount(const std::string& event) const {
    if (event.empty()) {
        size_t n = 0;
        for (const auto& [_,v] : m_impl->listeners) n += v.size();
        return n;
    }
    auto it = m_impl->listeners.find(event);
    return it != m_impl->listeners.end() ? it->second.size() : 0;
}

void EventDispatcher::dispatchRaw(const std::string& event,
                                   const std::any& payload)
{
    auto* vecPtr = m_impl->Get(event);
    if (!vecPtr) return;
    // Copy to allow listeners to unsubscribe from within callback
    std::vector<EventListener> copy = *vecPtr;
    std::vector<ListenerID> toRemove;
    for (auto& listener : copy) {
        if (listener.fn) listener.fn(payload);
        if (listener.oneShot) toRemove.push_back(listener.id);
    }
    for (auto id : toRemove) Unsubscribe(id);
}

void EventDispatcher::deferRaw(const std::string& event,
                                const std::any& payload)
{
    m_impl->deferred.push_back({event, payload});
}

void EventDispatcher::FlushDeferred() {
    // Move queue so callbacks can safely defer new events
    std::vector<DeferredEvent> batch;
    batch.swap(m_impl->deferred);
    for (auto& ev : batch) {
        dispatchRaw(ev.event, ev.payload);
    }
}

size_t EventDispatcher::DeferredCount() const {
    return m_impl->deferred.size();
}

void EventDispatcher::Clear() {
    m_impl->listeners.clear();
    m_impl->deferred.clear();
}

std::vector<std::string> EventDispatcher::RegisteredEvents() const {
    std::vector<std::string> out;
    for (const auto& [k,_] : m_impl->listeners) out.push_back(k);
    std::sort(out.begin(), out.end());
    return out;
}

} // namespace Core
