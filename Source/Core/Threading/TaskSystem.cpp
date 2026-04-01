#include "Core/Threading/TaskSystem.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <cassert>

namespace NF {

struct TaskSystem::Impl {
    std::vector<std::thread>               Workers;
    std::queue<std::packaged_task<void()>> Tasks;
    std::mutex                             Mutex;
    std::condition_variable                CV;
    bool                                   Stopping{false};

    void WorkerLoop() {
        for (;;) {
            std::packaged_task<void()> task;
            {
                std::unique_lock lock(Mutex);
                CV.wait(lock, [this] { return Stopping || !Tasks.empty(); });
                if (Stopping && Tasks.empty())
                    return;
                task = std::move(Tasks.front());
                Tasks.pop();
            }
            task();
        }
    }
};

TaskSystem::~TaskSystem() {
    if (m_Impl)
        Shutdown();
}

void TaskSystem::Init(uint32_t threadCount) {
    assert(!m_Impl && "TaskSystem::Init called more than once");

    if (threadCount == 0)
        threadCount = std::max(1u, std::thread::hardware_concurrency()); // hardware_concurrency() may return 0

    m_Impl = new Impl();
    m_Impl->Workers.reserve(threadCount);

    for (uint32_t i = 0; i < threadCount; ++i)
        m_Impl->Workers.emplace_back([this] { m_Impl->WorkerLoop(); });
}

void TaskSystem::Shutdown() {
    if (!m_Impl)
        return;

    {
        std::lock_guard lock(m_Impl->Mutex);
        m_Impl->Stopping = true;
    }
    m_Impl->CV.notify_all();

    for (auto& worker : m_Impl->Workers)
        if (worker.joinable())
            worker.join();

    delete m_Impl;
    m_Impl = nullptr;
}

std::future<void> TaskSystem::Submit(std::function<void()> task) {
    assert(m_Impl && "TaskSystem::Submit called before Init");

    std::packaged_task<void()> pt(std::move(task));
    auto future = pt.get_future();

    {
        std::lock_guard lock(m_Impl->Mutex);
        m_Impl->Tasks.push(std::move(pt));
    }
    m_Impl->CV.notify_one();

    return future;
}

} // namespace NF
