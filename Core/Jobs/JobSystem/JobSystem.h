#pragma once
/**
 * @file JobSystem.h
 * @brief Work-stealing thread-pool, JobHandle, parallel-for, dependency graph.
 *
 * Features:
 *   - Init(numThreads): launch worker threads (0 = hardware_concurrency)
 *   - Schedule(fn, priority) → JobHandle
 *   - ScheduleAfter(fn, dependencies, priority) → JobHandle
 *   - ParallelFor(count, fn(index)) → JobHandle (splits into per-thread batches)
 *   - Wait(handle) — block until job and all its dependents are done
 *   - WaitAll() — block until queue empty
 *   - IsComplete(handle) → bool
 *   - GetStats() → submitted, completed, stolen
 *   - Priorities: Low, Normal, High, Critical
 *   - Job local storage: each worker has per-thread scratch
 *   - Graceful Shutdown()
 *
 * Typical usage:
 * @code
 *   JobSystem js;
 *   js.Init(4);
 *   auto h1 = js.Schedule([](){ BuildNavMesh(); });
 *   auto h2 = js.ScheduleAfter([](){ AIPlanAll(); }, {h1});
 *   js.ParallelFor(1024, [&](uint32_t i){ particles[i].Tick(dt); });
 *   js.WaitAll();
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Core {

enum class JobPriority : uint8_t { Low=0, Normal, High, Critical };

using JobHandle  = uint64_t;
using JobFn      = std::function<void()>;
using ParallelFn = std::function<void(uint32_t index)>;

struct JobStats {
    uint64_t submitted {0};
    uint64_t completed {0};
    uint64_t stolen    {0};
};

class JobSystem {
public:
    JobSystem();
    ~JobSystem();

    void Init    (uint32_t numThreads=0);
    void Shutdown();

    JobHandle Schedule     (JobFn fn, JobPriority priority=JobPriority::Normal);
    JobHandle ScheduleAfter(JobFn fn,
                             const std::vector<JobHandle>& deps,
                             JobPriority priority=JobPriority::Normal);
    JobHandle ParallelFor  (uint32_t count, ParallelFn fn,
                             JobPriority priority=JobPriority::Normal);

    void Wait   (JobHandle handle);
    void WaitAll();
    bool IsComplete(JobHandle handle) const;

    uint32_t  ThreadCount() const;
    JobStats  GetStats   () const;
    void      ResetStats ();

    // Inline execution fallback (single-threaded when numThreads==0 explicitly)
    void SetFallbackSingleThread(bool enabled);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
