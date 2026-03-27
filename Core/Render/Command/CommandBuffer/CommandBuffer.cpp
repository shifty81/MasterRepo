#include "Core/Render/Command/CommandBuffer/CommandBuffer.h"
#include <algorithm>
#include <mutex>

namespace Core {

struct CommandBuffer::Impl {
    std::vector<RenderCommand> cmds;
    bool                       recording{false};
    mutable std::mutex         mtx;
};

CommandBuffer::CommandBuffer()  { m_impl = new Impl; }
CommandBuffer::~CommandBuffer() { delete m_impl; }

void CommandBuffer::Init    () {}
void CommandBuffer::Shutdown() { Reset(); }
void CommandBuffer::Reset   () {
    std::lock_guard<std::mutex> lk(m_impl->mtx);
    m_impl->cmds.clear();
}

void CommandBuffer::BeginRecording() { m_impl->recording = true;  }
void CommandBuffer::EndRecording  () { m_impl->recording = false; }

void CommandBuffer::Push(const RenderCommand& cmd) {
    m_impl->cmds.push_back(cmd);
}

void CommandBuffer::PushThreadSafe(const RenderCommand& cmd) {
    std::lock_guard<std::mutex> lk(m_impl->mtx);
    m_impl->cmds.push_back(cmd);
}

void CommandBuffer::Merge(const CommandBuffer& other) {
    for (auto& c : other.m_impl->cmds) m_impl->cmds.push_back(c);
}

void CommandBuffer::Sort() {
    std::stable_sort(m_impl->cmds.begin(), m_impl->cmds.end(),
        [](const RenderCommand& a, const RenderCommand& b){
            return a.sortKey < b.sortKey;
        });
}

void CommandBuffer::Execute(std::function<void(const RenderCommand&)> backend) {
    if (!backend) return;
    for (auto& c : m_impl->cmds) backend(c);
}

void CommandBuffer::Reserve(uint32_t n) {
    m_impl->cmds.reserve(n);
}

void CommandBuffer::SetSortKey(uint32_t index, uint64_t key) {
    if (index < (uint32_t)m_impl->cmds.size())
        m_impl->cmds[index].sortKey = key;
}

uint32_t CommandBuffer::GetCommandCount() const {
    return (uint32_t)m_impl->cmds.size();
}

const RenderCommand& CommandBuffer::GetCommand(uint32_t index) const {
    return m_impl->cmds[index];
}

} // namespace Core
