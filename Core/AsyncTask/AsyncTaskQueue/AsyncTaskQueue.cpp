#include "Core/AsyncTask/AsyncTaskQueue/AsyncTaskQueue.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Core {

struct TaskEntry {
    uint32_t   id{0};
    TaskDesc   desc;
    TaskStatus status{TaskStatus::Pending};
    float      progress{0.f};
    TaskResult result;

    std::function<void(TaskResult)>  onComplete;
    std::function<void(float)>       onProgress;
    std::function<void(std::string)> onError;
};

struct BatchEntry {
    uint32_t       batchId{0};
    std::vector<uint32_t> taskIds;
    std::function<void()> onAllDone;
};

struct ReadyCallback {
    uint32_t  taskId{0};
    std::function<void()> fn;
};

struct AsyncTaskQueue::Impl {
    std::unordered_map<uint32_t, TaskEntry> tasks;
    std::vector<BatchEntry>                 batches;
    std::vector<ReadyCallback>              pendingCallbacks;

    std::mutex              mu;
    std::condition_variable cv;
    std::vector<std::thread> workers;
    std::queue<uint32_t>    workQueue;
    bool                    stopping{false};
    uint32_t                nextId{1};
    uint32_t                nextBatchId{0x80000000u};

    std::atomic<uint32_t> runningCount{0};
    std::atomic<uint32_t> completedCount{0};

    void WorkerLoop() {
        while (true) {
            uint32_t taskId = 0;
            {
                std::unique_lock<std::mutex> lk(mu);
                cv.wait(lk, [this]{ return stopping || !workQueue.empty(); });
                if (stopping && workQueue.empty()) return;
                taskId = workQueue.front(); workQueue.pop();
            }

            TaskEntry* te = nullptr;
            {
                std::lock_guard<std::mutex> lk(mu);
                auto it = tasks.find(taskId);
                if (it == tasks.end()) continue;
                te = &it->second;
                if (te->status == TaskStatus::Cancelled) continue;
                te->status = TaskStatus::Running;
            }

            runningCount++;
            ProgressFn prog = [this, taskId](float p) {
                std::lock_guard<std::mutex> lk(mu);
                auto it = tasks.find(taskId);
                if (it != tasks.end()) {
                    it->second.progress = p;
                    if (it->second.onProgress) {
                        auto cb = it->second.onProgress;
                        pendingCallbacks.push_back({taskId, [cb,p]{ cb(p); }});
                    }
                }
            };

            TaskResult res;
            try { res = te->desc.work(prog); }
            catch(const std::exception& ex) {
                res.succeeded = false; res.errorMessage = ex.what();
            }

            {
                std::lock_guard<std::mutex> lk(mu);
                auto it = tasks.find(taskId);
                if (it != tasks.end()) {
                    it->second.result = res;
                    it->second.status = res.succeeded ? TaskStatus::Completed : TaskStatus::Failed;
                    it->second.progress = 1.f;
                    auto& e = it->second;
                    if (res.succeeded && e.onComplete) {
                        auto cb = e.onComplete; auto r = res;
                        pendingCallbacks.push_back({taskId,[cb,r]{ cb(r); }});
                    } else if (!res.succeeded && e.onError) {
                        auto cb = e.onError; auto msg = res.errorMessage;
                        pendingCallbacks.push_back({taskId,[cb,msg]{ cb(msg); }});
                    }
                }
            }
            runningCount--;
            completedCount++;

            // Check batches
            CheckBatches(taskId);
        }
    }

    void CheckBatches(uint32_t /*finishedId*/) {
        std::lock_guard<std::mutex> lk(mu);
        for (auto& b : batches) {
            bool allDone = true;
            for (auto tid : b.taskIds) {
                auto it = tasks.find(tid);
                if (it == tasks.end()) continue;
                if (it->second.status == TaskStatus::Pending ||
                    it->second.status == TaskStatus::Running) { allDone=false; break; }
            }
            if (allDone && b.onAllDone) {
                auto cb = b.onAllDone;
                pendingCallbacks.push_back({0,[cb]{ cb(); }});
                b.onAllDone = nullptr; // fire once
            }
        }
    }

    void Enqueue(uint32_t taskId) {
        // Check dependencies
        {
            std::lock_guard<std::mutex> lk(mu);
            auto it = tasks.find(taskId);
            if (it == tasks.end()) return;
            for (auto dep : it->second.desc.dependsOn) {
                auto dit = tasks.find(dep);
                if (dit != tasks.end() &&
                    dit->second.status != TaskStatus::Completed) return; // defer
            }
            workQueue.push(taskId);
        }
        cv.notify_one();
    }
};

AsyncTaskQueue::AsyncTaskQueue()  : m_impl(new Impl) {}
AsyncTaskQueue::~AsyncTaskQueue() { Shutdown(false); delete m_impl; }

void AsyncTaskQueue::Init(uint32_t workerThreads)
{
    m_impl->stopping = false;
    for (uint32_t i=0;i<workerThreads;i++)
        m_impl->workers.emplace_back([this]{ m_impl->WorkerLoop(); });
}

void AsyncTaskQueue::Shutdown(bool drain)
{
    if (drain) {
        // wait for queue to drain
        while (true) {
            std::unique_lock<std::mutex> lk(m_impl->mu);
            if (m_impl->workQueue.empty() && m_impl->runningCount==0) break;
            lk.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    {
        std::lock_guard<std::mutex> lk(m_impl->mu);
        m_impl->stopping = true;
    }
    m_impl->cv.notify_all();
    for (auto& w : m_impl->workers) if (w.joinable()) w.join();
    m_impl->workers.clear();
}

uint32_t AsyncTaskQueue::Submit(const TaskDesc& desc)
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    uint32_t id = m_impl->nextId++;
    TaskEntry te; te.id=id; te.desc=desc;
    m_impl->tasks[id] = te;
    m_impl->workQueue.push(id);
    m_impl->cv.notify_one();
    return id;
}

uint32_t AsyncTaskQueue::Submit(const std::string& name,
                                 std::function<TaskResult(ProgressFn)> work,
                                 TaskPriority priority)
{
    TaskDesc d; d.name=name; d.priority=priority; d.work=work;
    return Submit(d);
}

AsyncTaskQueue& AsyncTaskQueue::OnComplete(uint32_t id, std::function<void(TaskResult)> cb)
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    auto it = m_impl->tasks.find(id);
    if (it != m_impl->tasks.end()) it->second.onComplete = cb;
    return *this;
}

AsyncTaskQueue& AsyncTaskQueue::OnProgress(uint32_t id, std::function<void(float)> cb)
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    auto it = m_impl->tasks.find(id);
    if (it != m_impl->tasks.end()) it->second.onProgress = cb;
    return *this;
}

AsyncTaskQueue& AsyncTaskQueue::OnError(uint32_t id, std::function<void(std::string)> cb)
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    auto it = m_impl->tasks.find(id);
    if (it != m_impl->tasks.end()) it->second.onError = cb;
    return *this;
}

uint32_t AsyncTaskQueue::Batch(const std::vector<uint32_t>& taskIds,
                                 std::function<void()> onAllDone)
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    uint32_t bid = m_impl->nextBatchId++;
    BatchEntry b; b.batchId=bid; b.taskIds=taskIds; b.onAllDone=onAllDone;
    m_impl->batches.push_back(b);
    return bid;
}

void AsyncTaskQueue::Cancel(uint32_t id)
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    auto it = m_impl->tasks.find(id);
    if (it != m_impl->tasks.end()) it->second.status = TaskStatus::Cancelled;
}

void AsyncTaskQueue::CancelByTag(const std::string& tag)
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    for (auto& [id,te] : m_impl->tasks)
        if (te.desc.tag==tag) te.status=TaskStatus::Cancelled;
}

void AsyncTaskQueue::CancelAll()
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    for (auto& [id,te] : m_impl->tasks) te.status=TaskStatus::Cancelled;
}

TaskStatus AsyncTaskQueue::GetStatus(uint32_t id) const
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    auto it = m_impl->tasks.find(id);
    return it != m_impl->tasks.end() ? it->second.status : TaskStatus::Cancelled;
}

float AsyncTaskQueue::GetProgress(uint32_t id) const
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    auto it = m_impl->tasks.find(id);
    return it != m_impl->tasks.end() ? it->second.progress : 0.f;
}

bool AsyncTaskQueue::IsAlive(uint32_t id) const
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    auto it = m_impl->tasks.find(id);
    if (it==m_impl->tasks.end()) return false;
    auto s = it->second.status;
    return s==TaskStatus::Pending || s==TaskStatus::Running;
}

void AsyncTaskQueue::DrainCallbacks()
{
    std::vector<ReadyCallback> cbs;
    {
        std::lock_guard<std::mutex> lk(m_impl->mu);
        cbs.swap(m_impl->pendingCallbacks);
    }
    for (auto& rc : cbs) if (rc.fn) rc.fn();
}

uint32_t AsyncTaskQueue::PendingCount() const
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    uint32_t n=0;
    for (auto& [id,te] : m_impl->tasks) if (te.status==TaskStatus::Pending) n++;
    return n;
}

uint32_t AsyncTaskQueue::RunningCount()   const { return m_impl->runningCount.load(); }
uint32_t AsyncTaskQueue::CompletedCount() const { return m_impl->completedCount.load(); }

} // namespace Core
