#include "EditorCommandBus.h"

namespace atlas::tools {

void EditorCommandBus::Enqueue(const EditorCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pending.push_back(cmd);
}

void EditorCommandBus::Enqueue(EditorCommand&& cmd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pending.push_back(std::move(cmd));
}

void EditorCommandBus::Drain(std::vector<EditorCommand>& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    out.insert(out.end(),
               std::make_move_iterator(m_pending.begin()),
               std::make_move_iterator(m_pending.end()));
    m_pending.clear();
}

size_t EditorCommandBus::PendingCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pending.size();
}

void EditorCommandBus::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pending.clear();
}

void EditorCommandBus::RegisterHandler(EditorCommandType type, Handler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers[static_cast<uint8_t>(type)].push_back(std::move(handler));
}

void EditorCommandBus::Dispatch() {
    std::vector<EditorCommand> batch;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        batch.swap(m_pending);
    }
    for (const auto& cmd : batch) {
        auto it = m_handlers.find(static_cast<uint8_t>(cmd.type));
        if (it != m_handlers.end()) {
            for (const auto& handler : it->second) {
                handler(cmd);
            }
        }
    }
}

} // namespace atlas::tools
