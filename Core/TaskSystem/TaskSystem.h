#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace Core::TaskSystem {

/// Priority levels for tasks (higher numeric value = higher priority).
enum class TaskPriority : uint8_t {
    Low    = 0,
    Normal = 1,
    High   = 2,
    Critical = 3
};

/// Unique handle for a submitted task.
using TaskID = uint64_t;

/// Descriptor for a unit of work submitted to the TaskSystem.
struct Task {
    std::function<void()>   Work;
    TaskPriority            Priority = TaskPriority::Normal;
    std::string             Name;                     // Optional human-readable label.
    std::vector<TaskID>     Dependencies;              // Tasks that must complete first.
    std::function<void()>   OnComplete;                // Optional completion callback.
};

/// Multi-threaded task / job system with priority scheduling.
///
/// Maintains a configurable pool of worker threads that pull tasks from a
/// priority queue.  Tasks may declare dependencies on other tasks and will
/// not be scheduled until all dependencies have completed.
///
/// Usage:
///   Core::TaskSystem::TaskSystem ts;       // hardware_concurrency workers
///   auto id = ts.Submit({ .Work = []{ DoWork(); }, .Priority = TaskPriority::High });
///   ts.WaitForAll();
///   ts.Shutdown();                          // also called by destructor
class TaskSystem {
public:
    /// Construct and start worker threads.
    /// @param workerCount  Number of threads.  0 → std::thread::hardware_concurrency().
    explicit TaskSystem(unsigned int workerCount = 0);

    /// Shuts down the system, joining all workers.
    ~TaskSystem();

    TaskSystem(const TaskSystem&)            = delete;
    TaskSystem& operator=(const TaskSystem&) = delete;

    /// Submit a task for execution.  Returns a TaskID handle.
    [[nodiscard]] TaskID Submit(Task task);

    /// Block until every submitted task has completed.
    void WaitForAll();

    /// Signal all workers to finish and join threads.
    /// Safe to call multiple times.
    void Shutdown();

    /// Return the number of worker threads.
    [[nodiscard]] unsigned int GetWorkerCount() const noexcept { return static_cast<unsigned int>(m_Workers.size()); }

private:
    // --- Internal bookkeeping ---

    struct InternalTask {
        TaskID                  ID;
        Task                    Desc;
        std::shared_ptr<std::promise<void>> Promise;
    };

    struct PriorityCompare {
        bool operator()(const InternalTask& a, const InternalTask& b) const noexcept {
            return static_cast<uint8_t>(a.Desc.Priority) < static_cast<uint8_t>(b.Desc.Priority);
        }
    };

    void WorkerLoop();
    bool AreDependenciesMet(const InternalTask& task) const;

    // --- State ---

    std::vector<std::thread>                                          m_Workers;
    std::priority_queue<InternalTask, std::vector<InternalTask>, PriorityCompare> m_Queue;
    std::vector<InternalTask>                                         m_Deferred; // Waiting on deps.
    mutable std::mutex                                                m_Mutex;
    std::condition_variable                                           m_Condition;
    std::condition_variable                                           m_DoneCondition;
    std::atomic<bool>                                                 m_Shutdown{false};
    std::atomic<uint64_t>                                             m_NextID{1};
    std::atomic<uint64_t>                                             m_InFlight{0};
    std::unordered_map<TaskID, std::shared_future<void>>              m_Futures;
};

} // namespace Core::TaskSystem
