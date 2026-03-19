#include "EventBus.h"

namespace atlas {

SubscriptionId EventBus::Subscribe(const std::string& eventType, Callback cb) {
    if (!cb) return 0;

    std::lock_guard<std::mutex> lock(m_mutex);

    Subscription sub;
    sub.id        = m_nextId++;
    sub.eventType = eventType;
    sub.callback  = std::move(cb);

    if (eventType == "*") {
        m_wildcardSubs.push_back(std::move(sub));
    } else {
        m_subs[eventType].push_back(std::move(sub));
    }

    return sub.id;
}

void EventBus::Unsubscribe(SubscriptionId id) {
    if (id == 0) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    // Check wildcard list first.
    {
        auto& v = m_wildcardSubs;
        v.erase(std::remove_if(v.begin(), v.end(),
            [id](const Subscription& s) { return s.id == id; }),
            v.end());
    }

    // Check per-type lists.
    for (auto& [type, vec] : m_subs) {
        auto it = std::remove_if(vec.begin(), vec.end(),
            [id](const Subscription& s) { return s.id == id; });
        if (it != vec.end()) {
            vec.erase(it, vec.end());
            return;
        }
    }
}

void EventBus::Publish(const Event& event) {
    // Take a snapshot of the callbacks under the lock, then invoke
    // outside the lock so callbacks can safely call Subscribe/Publish.
    std::vector<Callback> toInvoke;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ++m_totalPublished;

        auto it = m_subs.find(event.type);
        if (it != m_subs.end()) {
            for (const auto& sub : it->second) {
                toInvoke.push_back(sub.callback);
            }
        }
        for (const auto& sub : m_wildcardSubs) {
            toInvoke.push_back(sub.callback);
        }
    }

    for (auto& cb : toInvoke) {
        cb(event);
    }
}

void EventBus::Enqueue(const Event& event) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push_back(event);
}

void EventBus::Flush() {
    // Move the queue out so that events enqueued during dispatch are
    // deferred to the *next* Flush() call.
    std::vector<Event> pending;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        pending = std::move(m_queue);
        m_queue.clear();
    }

    for (const auto& event : pending) {
        Publish(event);
    }
}

size_t EventBus::SubscriptionCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = m_wildcardSubs.size();
    for (const auto& [type, vec] : m_subs) {
        count += vec.size();
    }
    return count;
}

size_t EventBus::QueueSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

void EventBus::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_subs.clear();
    m_wildcardSubs.clear();
    m_queue.clear();
    m_nextId = 1;
    m_totalPublished = 0;
}

} // namespace atlas
