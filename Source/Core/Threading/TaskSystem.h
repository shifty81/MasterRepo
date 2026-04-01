#pragma once
#include <cstdint>
#include <functional>
#include <future>

namespace NF {

/// @brief Fixed-size thread pool that executes submitted tasks asynchronously.
class TaskSystem {
public:
    TaskSystem() = default;
    ~TaskSystem();

    // Non-copyable, non-movable (owns worker threads)
    TaskSystem(const TaskSystem&)            = delete;
    TaskSystem& operator=(const TaskSystem&) = delete;

    /// @brief Spawn worker threads and begin accepting tasks.
    /// @param threadCount Number of worker threads to create.
    ///        Pass 0 to use std::thread::hardware_concurrency().
    void Init(uint32_t threadCount);

    /// @brief Drain all pending tasks, join all workers, and release resources.
    void Shutdown();

    /// @brief Enqueue a callable for asynchronous execution on a worker thread.
    /// @param task Callable with signature void().
    /// @return A future that becomes ready once the task has completed.
    [[nodiscard]] std::future<void> Submit(std::function<void()> task);

private:
    struct Impl;
    Impl* m_Impl{nullptr};
};

} // namespace NF
