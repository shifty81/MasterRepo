#include "Core/JobSystem/JobSystem.h"
#include <algorithm>
#include <stdexcept>

namespace Core {

// ── Singleton ──────────────────────────────────────────────────

JobSystem& JobSystem::Get() {
    static JobSystem instance;
    return instance;
}

// ── Lifecycle ──────────────────────────────────────────────────

void JobSystem::Initialise(uint32_t workerCount) {
    if (m_running.exchange(true)) return;  // already running

    uint32_t count = (workerCount == 0)
        ? std::max(1u, std::thread::hardware_concurrency())
        : workerCount;

    m_workers.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        m_workers.emplace_back([this]{ WorkerLoop(); });
    }
}

void JobSystem::Shutdown() {
    {
        std::lock_guard<std::mutex> lk(m_queueMutex);
        m_running = false;
    }
    m_cv.notify_all();
    for (auto& t : m_workers) {
        if (t.joinable()) t.join();
    }
    m_workers.clear();
}

// ── Submit ─────────────────────────────────────────────────────

JobHandle JobSystem::Submit(std::function<void()> fn,
                             JobPriority priority,
                             const std::string& tag) {
    auto job       = std::make_shared<Job>();
    job->id        = NextId();
    job->fn        = std::move(fn);
    job->priority  = priority;
    job->tag       = tag;
    job->status.store(JobStatus::Queued);

    {
        std::lock_guard<std::mutex> lk(m_jobsMutex);
        m_jobs[job->id] = job;
    }
    {
        std::lock_guard<std::mutex> lk(m_queueMutex);
        m_queue.push(job);
    }
    m_cv.notify_one();

    return JobHandle{job->id};
}

JobHandle JobSystem::SubmitBatch(std::vector<std::function<void()>> fns,
                                  JobPriority priority) {
    // Submit each fn; return the handle of the last one as the "batch done" signal
    JobHandle last{};
    for (auto& fn : fns) {
        last = Submit(std::move(fn), priority, "batch");
    }
    return last;
}

void JobSystem::AddDependency(JobHandle dependent, JobHandle prerequisite) {
    std::lock_guard<std::mutex> lk(m_jobsMutex);
    auto it = m_jobs.find(dependent.id);
    if (it != m_jobs.end()) {
        it->second->prerequisites.push_back(prerequisite.id);
    }
}

// ── Waiting ────────────────────────────────────────────────────

void JobSystem::WaitAll() {
    while (PendingCount() > 0 || RunningCount() > 0) {
        std::this_thread::yield();
    }
}

bool JobSystem::WaitFor(JobHandle handle, uint32_t timeoutMs) {
    using Clock = std::chrono::steady_clock;
    auto deadline = (timeoutMs > 0)
        ? Clock::now() + std::chrono::milliseconds(timeoutMs)
        : Clock::time_point::max();

    while (Clock::now() < deadline) {
        JobStatus s = GetStatus(handle);
        if (s == JobStatus::Completed || s == JobStatus::Failed ||
            s == JobStatus::Cancelled) {
            return true;
        }
        std::this_thread::yield();
    }
    return false;
}

// ── Cancel ─────────────────────────────────────────────────────

bool JobSystem::Cancel(JobHandle handle) {
    std::lock_guard<std::mutex> lk(m_jobsMutex);
    auto it = m_jobs.find(handle.id);
    if (it == m_jobs.end()) return false;
    JobStatus expected = JobStatus::Queued;
    return it->second->status.compare_exchange_strong(expected, JobStatus::Cancelled);
}

// ── Queries ────────────────────────────────────────────────────

JobStatus JobSystem::GetStatus(JobHandle handle) const {
    std::lock_guard<std::mutex> lk(m_jobsMutex);
    auto it = m_jobs.find(handle.id);
    if (it == m_jobs.end()) return JobStatus::Completed;
    return it->second->status.load();
}

uint32_t JobSystem::PendingCount() const {
    std::lock_guard<std::mutex> lk(m_queueMutex);
    return static_cast<uint32_t>(m_queue.size());
}

uint32_t JobSystem::RunningCount() const {
    std::lock_guard<std::mutex> lk(m_jobsMutex);
    uint32_t n = 0;
    for (auto& [id, job] : m_jobs) {
        if (job->status.load() == JobStatus::Running) ++n;
    }
    return n;
}

uint32_t JobSystem::WorkerCount() const {
    return static_cast<uint32_t>(m_workers.size());
}

// ── Worker loop ────────────────────────────────────────────────

void JobSystem::WorkerLoop() {
    while (true) {
        std::shared_ptr<Job> job;
        {
            std::unique_lock<std::mutex> lk(m_queueMutex);
            m_cv.wait(lk, [this]{
                return !m_running.load() || !m_queue.empty();
            });
            if (!m_running.load() && m_queue.empty()) break;
            if (m_queue.empty()) continue;
            job = m_queue.top();
            m_queue.pop();
        }

        // Skip cancelled jobs
        if (job->status.load() == JobStatus::Cancelled) continue;

        // If dependencies are not yet met, defer to a short sleep then re-enqueue.
        // The job is placed back without holding m_queueMutex during the sleep,
        // which avoids blocking the main queue while waiting.
        if (!AreDependenciesMet(*job)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            {
                std::lock_guard<std::mutex> lk(m_queueMutex);
                m_queue.push(job);
            }
            m_cv.notify_one();
            continue;
        }

        job->status.store(JobStatus::Running);
        try {
            job->fn();
            job->status.store(JobStatus::Completed);
        } catch (...) {
            job->status.store(JobStatus::Failed);
        }
    }
}

bool JobSystem::AreDependenciesMet(const Job& job) const {
    std::lock_guard<std::mutex> lk(m_jobsMutex);
    for (uint64_t prereqId : job.prerequisites) {
        auto it = m_jobs.find(prereqId);
        if (it == m_jobs.end()) continue;
        JobStatus s = it->second->status.load();
        if (s != JobStatus::Completed && s != JobStatus::Failed &&
            s != JobStatus::Cancelled) {
            return false;
        }
    }
    return true;
}

uint64_t JobSystem::NextId() {
    return ++m_idCounter;
}

} // namespace Core
