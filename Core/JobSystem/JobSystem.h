#pragma once
#include <functional>
#include <future>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>

namespace Core {

// ──────────────────────────────────────────────────────────────
// Job priority
// ──────────────────────────────────────────────────────────────

enum class JobPriority { Low = 0, Normal = 1, High = 2, Critical = 3 };

// ──────────────────────────────────────────────────────────────
// Job handle — lightweight token for tracking a submitted job
// ──────────────────────────────────────────────────────────────

struct JobHandle {
    uint64_t id   = 0;
    bool     valid() const { return id != 0; }
};

// ──────────────────────────────────────────────────────────────
// Job status
// ──────────────────────────────────────────────────────────────

enum class JobStatus { Queued, Running, Completed, Failed, Cancelled };

// ──────────────────────────────────────────────────────────────
// JobSystem — advanced multi-thread CPU/GPU-ready task scheduler
// ──────────────────────────────────────────────────────────────

class JobSystem {
public:
    // Lifecycle
    void Initialise(uint32_t workerCount = 0);  // 0 = auto (hardware_concurrency)
    void Shutdown();

    // Submit a callable job; returns a handle for tracking
    JobHandle Submit(std::function<void()> fn,
                     JobPriority priority   = JobPriority::Normal,
                     const std::string& tag = "");

    // Submit a job that returns a value
    template<typename Fn>
    auto SubmitFuture(Fn&& fn, JobPriority priority = JobPriority::Normal)
        -> std::future<decltype(fn())>;

    // Submit a batch of jobs; returned handle becomes "done" when all finish
    JobHandle SubmitBatch(std::vector<std::function<void()>> fns,
                          JobPriority priority = JobPriority::Normal);

    // Job dependencies — run `dependent` only after `prerequisite` completes
    void AddDependency(JobHandle dependent, JobHandle prerequisite);

    // Waiting
    void WaitAll();
    bool WaitFor(JobHandle handle, uint32_t timeoutMs = 0);

    // Cancellation (best-effort for queued jobs)
    bool Cancel(JobHandle handle);

    // Queries
    JobStatus  GetStatus(JobHandle handle) const;
    uint32_t   PendingCount()  const;
    uint32_t   RunningCount()  const;
    uint32_t   WorkerCount()   const;

    // Singleton helper
    static JobSystem& Get();

private:
    struct Job {
        uint64_t              id       = 0;
        std::function<void()> fn;
        JobPriority           priority = JobPriority::Normal;
        std::string           tag;
        std::atomic<JobStatus> status  {JobStatus::Queued};
        std::vector<uint64_t> prerequisites;
    };

    struct PriorityCmp {
        bool operator()(const std::shared_ptr<Job>& a,
                        const std::shared_ptr<Job>& b) const {
            return static_cast<int>(a->priority) < static_cast<int>(b->priority);
        }
    };

    void    WorkerLoop();
    bool    AreDependenciesMet(const Job& job) const;
    uint64_t NextId();

    std::vector<std::thread>   m_workers;
    std::priority_queue<std::shared_ptr<Job>,
                        std::vector<std::shared_ptr<Job>>,
                        PriorityCmp>        m_queue;
    mutable std::mutex         m_queueMutex;
    std::condition_variable    m_cv;
    std::atomic<bool>          m_running   {false};
    std::atomic<uint64_t>      m_idCounter {0};
    mutable std::mutex         m_jobsMutex;
    std::unordered_map<uint64_t, std::shared_ptr<Job>> m_jobs;
};

// ── template implementation ────────────────────────────────────

template<typename Fn>
auto JobSystem::SubmitFuture(Fn&& fn, JobPriority priority)
    -> std::future<decltype(fn())>
{
    using RetT = decltype(fn());
    auto promise = std::make_shared<std::promise<RetT>>();
    auto future  = promise->get_future();
    Submit([p = std::move(promise), f = std::forward<Fn>(fn)]() mutable {
        try {
            if constexpr (std::is_void_v<RetT>) {
                f();
                p->set_value();
            } else {
                p->set_value(f());
            }
        } catch (...) {
            p->set_exception(std::current_exception());
        }
    }, priority);
    return future;
}

} // namespace Core
