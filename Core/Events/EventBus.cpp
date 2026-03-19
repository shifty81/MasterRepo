#include "Core/Events/EventBus.h"

#include <algorithm>

namespace Core {

HandlerID EventBus::SubscribeByType(EventTypeID typeID,
                                    std::function<void(Event&)> callback) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    HandlerID id = m_NextID++;
    m_Handlers[typeID].push_back({id, std::move(callback)});
    m_HandlerTypeMap[id] = typeID;
    return id;
}

void EventBus::Unsubscribe(HandlerID id) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto typeIt = m_HandlerTypeMap.find(id);
    if (typeIt == m_HandlerTypeMap.end()) {
        return;
    }

    auto& records = m_Handlers[typeIt->second];
    auto it = std::find_if(records.begin(), records.end(),
                           [id](const HandlerRecord& r) { return r.ID == id; });
    if (it != records.end()) {
        records.erase(it);
    }

    m_HandlerTypeMap.erase(typeIt);
}

void EventBus::Publish(Event& event) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_Handlers.find(event.GetTypeID());
    if (it == m_Handlers.end()) {
        return;
    }

    for (auto& record : it->second) {
        record.Callback(event);
        if (event.Handled) {
            break;
        }
    }
}

} // namespace Core
