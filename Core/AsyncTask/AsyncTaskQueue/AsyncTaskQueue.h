#pragma once
/**
 * @file AsyncTaskQueue.h
 * @brief Background task queue with priorities, progress reporting, and cancellation.
 *
 * Features:
 *   - Submit typed tasks to a thread pool; results returned on main thread
 *   - Priority levels: Critical, High, Normal, Low, Background
 *   - Per-task progress [0,1] report; query from any thread
 *   - Cancellation token: cancel individual tasks or all tasks of a tag
 *   - Dependency graph: task B starts only after task A completes
 *   - On-complete, on-error, on-progress callbacks (always fired on main thread)
 *   - Task batches: submit N tasks, get one combined on-all-done callback
 *   - Max-concurrency limit per priority tier
 *   - Graceful shutdown: drain queue or cancel pending
 *
 * Typical usage:
 * @code
 *   AsyncTaskQueue q;
 *   q.Init(4);  // 4 worker threads
 *   uint32_t id = q.Submit({
 *       "load_texture",
 *       TaskPriority::High,
 *       [](ProgressFn prog){ return LoadTexture("tex.png", prog); }
 *   });
 *   q.OnComplete(id, [](TaskResult r){ UploadToGPU(r.data); });
 *   // main loop
 *   q.DrainCallbacks();  // fires completed callbacks
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Core {

enum class TaskPriority : uint8_t { Critical=0, High, Normal, Low, Background };
enum class TaskStatus   : uint8_t { Pending=0, Running, Completed, Failed, Cancelled };

using ProgressFn = std::function<void(float)>;  ///< 0–1 progress report

struct TaskResult {
    bool        succeeded{false};
    std::string errorMessage;
    std::shared_ptr<void> data;  ///< caller casts to expected type
};

struct TaskDesc {
    std::string   name;
    std::string   tag;                     ///< for bulk-cancel by tag
    TaskPriority  priority{TaskPriority::Normal};
    std::function<TaskResult(ProgressFn)> work;
    std::vector<uint32_t> dependsOn;       ///< task IDs that must complete first
};

class AsyncTaskQueue {
public:
    AsyncTaskQueue();
    ~AsyncTaskQueue();

    void Init(uint32_t workerThreads=2);
    void Shutdown(bool drain=false);

    // Submit
    uint32_t Submit(const TaskDesc& desc);
    uint32_t Submit(const std::string& name, std::function<TaskResult(ProgressFn)> work,
                    TaskPriority priority=TaskPriority::Normal);

    // Callbacks (always called on DrainCallbacks() thread = main thread)
    AsyncTaskQueue& OnComplete(uint32_t id, std::function<void(TaskResult)> cb);
    AsyncTaskQueue& OnProgress(uint32_t id, std::function<void(float)>    cb);
    AsyncTaskQueue& OnError   (uint32_t id, std::function<void(std::string)> cb);

    // Batch
    uint32_t Batch(const std::vector<uint32_t>& taskIds,
                   std::function<void()> onAllDone);

    // Control
    void     Cancel(uint32_t id);
    void     CancelByTag(const std::string& tag);
    void     CancelAll();

    // Query
    TaskStatus GetStatus  (uint32_t id) const;
    float      GetProgress(uint32_t id) const;
    bool       IsAlive    (uint32_t id) const;

    // Main-thread pump
    void DrainCallbacks();

    // Stats
    uint32_t PendingCount()  const;
    uint32_t RunningCount()  const;
    uint32_t CompletedCount()const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
