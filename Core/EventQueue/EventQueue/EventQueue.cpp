#include "Core/EventQueue/EventQueue/EventQueue.h"
#include <algorithm>

namespace Core {

EventQueue::EventQueue()  {}
EventQueue::~EventQueue() { Shutdown(); }
void EventQueue::Init()     {}
void EventQueue::Shutdown() { std::lock_guard<std::mutex> lk(m_enqueueMu); m_queue.clear(); }

void EventQueue::EnqueueEnvelope(EventEnvelope env)
{
    std::lock_guard<std::mutex> lk(m_enqueueMu);
    m_queue.push_back(std::move(env));
}

void EventQueue::FireListeners(const std::type_index& type, const void* raw)
{
    auto it = m_listeners.find(type);
    if (it == m_listeners.end()) return;

    // Copy list so listeners can safely unsubscribe during iteration
    auto listeners = it->second;
    std::sort(listeners.begin(), listeners.end(),
              [](const ListenerEntry& a, const ListenerEntry& b){
                  return a.priority > b.priority;
              });
    for (auto& entry : listeners) {
        if (entry.fn) entry.fn(raw);
    }
}

void EventQueue::Flush()
{
    std::vector<EventEnvelope> toProcess;
    {
        std::lock_guard<std::mutex> lk(m_enqueueMu);
        toProcess.swap(m_queue);
    }
    for (auto& env : toProcess) {
        if (env.data) FireListeners(env.type, env.data.get());
    }
}

void EventQueue::Clear()
{
    std::lock_guard<std::mutex> lk(m_enqueueMu);
    m_queue.clear();
}

EventSubscription EventQueue::SubscribeInternal(const std::type_index& type,
                                                  std::function<void(const void*)> fn,
                                                  int32_t priority)
{
    uint32_t id = m_nextSubId++;
    m_listeners[type].push_back({id, priority, fn});
    return EventSubscription{id};
}

void EventQueue::Unsubscribe(EventSubscription token)
{
    if (!token) return;
    for (auto& [type, entries] : m_listeners) {
        auto& v = entries;
        v.erase(std::remove_if(v.begin(), v.end(),
                               [&](const ListenerEntry& e){ return e.id==token.id; }),
                v.end());
    }
}

uint32_t EventQueue::QueuedCount() const
{
    std::lock_guard<std::mutex> lk(const_cast<std::mutex&>(m_enqueueMu));
    return (uint32_t)m_queue.size();
}

} // namespace Core
