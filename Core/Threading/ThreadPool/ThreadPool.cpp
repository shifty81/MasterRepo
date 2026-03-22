#include "Core/Threading/ThreadPool/ThreadPool.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>
#include <atomic>
#include <stdexcept>

namespace Core {

// ── Task entry ────────────────────────────────────────────────────────────
struct Task {
    std::function<void()> fn;
    TaskPriority          priority{TaskPriority::Normal};

    bool operator<(const Task& o) const {
        // Higher priority value = lower urgency; use > so High(0) comes first
        return static_cast<int>(priority) > static_cast<int>(o.priority);
    }
};

// ── Impl ─────────────────────────────────────────────────────────────────
struct ThreadPool::Impl {
    std::priority_queue<Task>    queue;
    std::vector<std::thread>     workers;
    std::mutex                   mtx;
    std::condition_variable      cv;
    std::condition_variable      idleCv;
    std::atomic<bool>            shutdown{false};
    std::atomic<bool>            paused{false};
    std::atomic<size_t>          activeWorkers{0};

    // Stats
    std::atomic<uint64_t> submitted{0};
    std::atomic<uint64_t> completed{0};
    std::atomic<uint64_t> failed{0};
    std::atomic<size_t>   peakQueueDepth{0};

    void workerLoop() {
        while (true) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this] {
                    return shutdown.load() ||
                           (!paused.load() && !queue.empty());
                });
                if (shutdown.load() && queue.empty()) return;
                if (paused.load()) continue;
                task = queue.top();
                queue.pop();
            }
            ++activeWorkers;
            try { task.fn(); ++completed; }
            catch (...) { ++failed; }
            --activeWorkers;
            idleCv.notify_all();
        }
    }
};

ThreadPool::ThreadPool(size_t numThreads) : m_impl(new Impl()) {
    if (numThreads == 0)
        numThreads = std::max(1u, std::thread::hardware_concurrency());
    m_impl->workers.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i)
        m_impl->workers.emplace_back([this] { m_impl->workerLoop(); });
}

ThreadPool::~ThreadPool() {
    if (!m_impl->shutdown.load()) Shutdown();
    delete m_impl;
}

void ThreadPool::SubmitImpl(std::function<void()> fn, TaskPriority priority) {
    if (m_impl->shutdown.load())
        throw std::runtime_error("ThreadPool: submit after shutdown");
    {
        std::lock_guard<std::mutex> lock(m_impl->mtx);
        Task t;
        t.fn       = std::move(fn);
        t.priority = priority;
        m_impl->queue.push(std::move(t));
        ++m_impl->submitted;
        size_t depth = m_impl->queue.size();
        size_t peak  = m_impl->peakQueueDepth.load();
        while (depth > peak &&
               !m_impl->peakQueueDepth.compare_exchange_weak(peak, depth))
        {}
    }
    m_impl->cv.notify_one();
}

void ThreadPool::SubmitVoid(std::function<void()> fn, TaskPriority priority) {
    SubmitImpl(std::move(fn), priority);
}

void ThreadPool::Pause() {
    m_impl->paused.store(true);
}

void ThreadPool::Resume() {
    m_impl->paused.store(false);
    m_impl->cv.notify_all();
}

bool ThreadPool::IsPaused() const { return m_impl->paused.load(); }

void ThreadPool::WaitAll() {
    std::unique_lock<std::mutex> lock(m_impl->mtx);
    m_impl->idleCv.wait(lock, [this] {
        return m_impl->queue.empty() && m_impl->activeWorkers.load() == 0;
    });
}

void ThreadPool::Shutdown() {
    m_impl->shutdown.store(true);
    m_impl->cv.notify_all();
    for (auto& t : m_impl->workers)
        if (t.joinable()) t.join();
}

bool ThreadPool::IsShutdown() const { return m_impl->shutdown.load(); }

size_t ThreadPool::ThreadCount() const { return m_impl->workers.size(); }

size_t ThreadPool::QueueDepth() const {
    std::lock_guard<std::mutex> lock(m_impl->mtx);
    return m_impl->queue.size();
}

ThreadPoolStats ThreadPool::Stats() const {
    ThreadPoolStats s;
    s.submitted       = m_impl->submitted.load();
    s.completed       = m_impl->completed.load();
    s.failed          = m_impl->failed.load();
    s.queueDepth      = QueueDepth();
    s.peakQueueDepth  = m_impl->peakQueueDepth.load();
    s.activeWorkers   = m_impl->activeWorkers.load();
    s.threadCount     = m_impl->workers.size();
    return s;
}

void ThreadPool::ResetStats() {
    m_impl->submitted.store(0);
    m_impl->completed.store(0);
    m_impl->failed.store(0);
    m_impl->peakQueueDepth.store(0);
}

} // namespace Core
