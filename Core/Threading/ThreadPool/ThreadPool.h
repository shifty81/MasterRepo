#pragma once
/**
 * @file ThreadPool.h
 * @brief Fixed-size thread pool with task queue, futures, and priority support.
 *
 * ThreadPool manages a pool of N worker threads that pull tasks from a
 * priority-ordered work queue:
 *
 *   - Submit<F>(fn, priority): enqueue a callable; returns std::future<ReturnType>.
 *   - SubmitVoid(fn, priority): enqueue a void callable; no future overhead.
 *   - Priority: High (0) > Normal (1) > Low (2); tasks at the same priority
 *     are processed FIFO.
 *   - WaitAll(): block until the queue is drained and all workers are idle.
 *   - Pause() / Resume(): suspend/resume task dispatch without stopping threads.
 *   - Shutdown(): finish in-flight tasks, drain the queue, and join all threads.
 *   - Stats: submitted, completed, failed, peak queue depth, active workers.
 *
 * Usage:
 *   Core::ThreadPool pool(4);
 *   auto fut = pool.Submit([]{ return expensiveWork(); });
 *   int result = fut.get();
 */

#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <thread>
#include <atomic>
#include <memory>
#include <type_traits>

namespace Core {

// ── Task priority ─────────────────────────────────────────────────────────
enum class TaskPriority : uint8_t { High = 0, Normal = 1, Low = 2 };

// ── Pool stats ────────────────────────────────────────────────────────────
struct ThreadPoolStats {
    uint64_t submitted{0};
    uint64_t completed{0};
    uint64_t failed{0};
    size_t   queueDepth{0};
    size_t   peakQueueDepth{0};
    size_t   activeWorkers{0};
    size_t   threadCount{0};
};

// ── Pool ──────────────────────────────────────────────────────────────────
class ThreadPool {
public:
    /// Create a pool with `numThreads` workers.
    /// If numThreads == 0, uses std::thread::hardware_concurrency().
    explicit ThreadPool(size_t numThreads = 0);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // ── task submission ───────────────────────────────────────
    /**
     * Enqueue a callable and return a future for its result.
     * Throws std::runtime_error if the pool has been shut down.
     */
    template<typename F, typename... Args>
    auto Submit(F&& fn, TaskPriority priority = TaskPriority::Normal,
                Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using R = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<R()>>(
            [fn = std::forward<F>(fn),
             tup = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                return std::apply(std::move(fn), std::move(tup));
            });
        std::future<R> fut = task->get_future();
        SubmitImpl([task]{ (*task)(); }, priority);
        return fut;
    }

    /// Enqueue a void callable without future overhead.
    void SubmitVoid(std::function<void()> fn,
                    TaskPriority priority = TaskPriority::Normal);

    // ── control ───────────────────────────────────────────────
    void Pause();
    void Resume();
    bool IsPaused() const;

    /// Block until queue is empty and all workers are idle.
    void WaitAll();

    /// Finish in-flight tasks, drain queue, join threads.
    void Shutdown();
    bool IsShutdown() const;

    // ── info ──────────────────────────────────────────────────
    size_t          ThreadCount() const;
    size_t          QueueDepth()  const;
    ThreadPoolStats Stats()       const;
    void            ResetStats();

private:
    void SubmitImpl(std::function<void()> fn, TaskPriority priority);

    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
