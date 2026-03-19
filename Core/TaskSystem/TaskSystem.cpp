#include "Core/TaskSystem/TaskSystem.h"

#include <algorithm>

namespace Core::TaskSystem {

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

TaskSystem::TaskSystem(unsigned int workerCount)
{
    if (workerCount == 0) {
        workerCount = std::thread::hardware_concurrency();
        if (workerCount == 0) workerCount = 2; // Fallback.
    }

    m_Workers.reserve(workerCount);
    for (unsigned int i = 0; i < workerCount; ++i) {
        m_Workers.emplace_back(&TaskSystem::WorkerLoop, this);
    }
}

TaskSystem::~TaskSystem()
{
    Shutdown();
}

// ---------------------------------------------------------------------------
// Submit
// ---------------------------------------------------------------------------

TaskID TaskSystem::Submit(Task task)
{
    const TaskID id = m_NextID.fetch_add(1);

    auto promise = std::make_shared<std::promise<void>>();
    auto future  = promise->get_future().share();

    InternalTask internal{ id, std::move(task), std::move(promise) };

    {
        std::lock_guard lock(m_Mutex);
        m_Futures[id] = future;
        m_InFlight.fetch_add(1);

        if (internal.Desc.Dependencies.empty()) {
            m_Queue.push(std::move(internal));
        } else {
            m_Deferred.push_back(std::move(internal));
        }
    }
    m_Condition.notify_one();
    return id;
}

// ---------------------------------------------------------------------------
// WaitForAll
// ---------------------------------------------------------------------------

void TaskSystem::WaitForAll()
{
    std::unique_lock lock(m_Mutex);
    m_DoneCondition.wait(lock, [this] {
        return m_InFlight.load() == 0;
    });
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

void TaskSystem::Shutdown()
{
    {
        std::lock_guard lock(m_Mutex);
        if (m_Shutdown.exchange(true)) return; // Already shut down.
    }
    m_Condition.notify_all();

    for (auto& worker : m_Workers) {
        if (worker.joinable()) worker.join();
    }
}

// ---------------------------------------------------------------------------
// WorkerLoop
// ---------------------------------------------------------------------------

void TaskSystem::WorkerLoop()
{
    for (;;) {
        InternalTask task{};
        {
            std::unique_lock lock(m_Mutex);
            m_Condition.wait(lock, [this] {
                return m_Shutdown.load() || !m_Queue.empty();
            });

            if (m_Shutdown.load() && m_Queue.empty()) return;
            if (m_Queue.empty()) continue;

            task = std::move(const_cast<InternalTask&>(m_Queue.top()));
            m_Queue.pop();
        }

        // Execute the work.
        if (task.Desc.Work) {
            task.Desc.Work();
        }

        // Mark completion.
        if (task.Promise) {
            task.Promise->set_value();
        }

        // Invoke optional callback.
        if (task.Desc.OnComplete) {
            task.Desc.OnComplete();
        }

        // Check deferred tasks whose deps may now be met.
        {
            std::lock_guard lock(m_Mutex);
            auto it = m_Deferred.begin();
            while (it != m_Deferred.end()) {
                if (AreDependenciesMet(*it)) {
                    m_Queue.push(std::move(*it));
                    it = m_Deferred.erase(it);
                } else {
                    ++it;
                }
            }
        }

        m_InFlight.fetch_sub(1);
        m_DoneCondition.notify_all();
        m_Condition.notify_one();
    }
}

// ---------------------------------------------------------------------------
// AreDependenciesMet
// ---------------------------------------------------------------------------

bool TaskSystem::AreDependenciesMet(const InternalTask& task) const
{
    for (TaskID dep : task.Desc.Dependencies) {
        auto it = m_Futures.find(dep);
        if (it == m_Futures.end()) continue; // Unknown dep treated as met.
        if (it->second.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            return false;
        }
    }
    return true;
}

} // namespace Core::TaskSystem
