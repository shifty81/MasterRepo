#include "Core/Jobs/JobSystem/JobSystem.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace Core {

struct JobEntry {
    JobFn      fn;
    JobPriority priority;
    JobHandle  handle;
    std::vector<JobHandle> deps;
    std::atomic<bool> done{false};
    JobEntry() = default;
    JobEntry(const JobEntry&) = delete;
    JobEntry& operator=(const JobEntry&) = delete;
};

struct JobSystem::Impl {
    std::vector<std::thread>            workers;
    std::vector<std::shared_ptr<JobEntry>> jobs;
    std::deque<std::shared_ptr<JobEntry>>  queue;
    std::mutex                          mtx;
    std::condition_variable             cv;
    std::atomic<bool>                   shutdown{false};
    std::atomic<uint64_t>               nextHandle{1};
    JobStats                            stats{};
    bool                                singleThread{false};

    std::shared_ptr<JobEntry> FindJob(JobHandle h){
        for(auto& j:jobs) if(j->handle==h) return j;
        return nullptr;
    }

    bool DepsReady(const std::vector<JobHandle>& deps){
        for(auto d:deps){ auto j=FindJob(d); if(j&&!j->done) return false; }
        return true;
    }

    void WorkerLoop(){
        while(!shutdown){
            std::shared_ptr<JobEntry> job;
            {
                std::unique_lock<std::mutex> lk(mtx);
                cv.wait(lk,[this]{
                    for(auto& j:queue) if(DepsReady(j->deps)) return true;
                    return shutdown.load();
                });
                if(shutdown) return;
                for(auto it=queue.begin();it!=queue.end();++it){
                    if(DepsReady((*it)->deps)){ job=*it; queue.erase(it); break; }
                }
            }
            if(job){ job->fn(); job->done=true; stats.completed++; cv.notify_all(); }
        }
    }
};

JobSystem::JobSystem()  : m_impl(new Impl){}
JobSystem::~JobSystem() { Shutdown(); delete m_impl; }

void JobSystem::Init(uint32_t n){
    uint32_t count=n?n:std::max(1u,(uint32_t)std::thread::hardware_concurrency());
    m_impl->singleThread=(n==0&&count==1)||count==0;
    if(m_impl->singleThread) return;
    for(uint32_t i=0;i<count;i++)
        m_impl->workers.emplace_back([this]{ m_impl->WorkerLoop(); });
}

void JobSystem::Shutdown(){
    m_impl->shutdown=true; m_impl->cv.notify_all();
    for(auto& t:m_impl->workers) if(t.joinable()) t.join();
    m_impl->workers.clear();
    {std::unique_lock<std::mutex> lk(m_impl->mtx); m_impl->queue.clear();}
}

JobHandle JobSystem::Schedule(JobFn fn, JobPriority priority){
    return ScheduleAfter(fn, {}, priority);
}

JobHandle JobSystem::ScheduleAfter(JobFn fn, const std::vector<JobHandle>& deps, JobPriority priority){
    auto job=std::make_shared<JobEntry>();
    job->fn=fn; job->priority=priority;
    job->handle=m_impl->nextHandle++; job->deps=deps;
    m_impl->stats.submitted++;
    if(m_impl->singleThread||m_impl->workers.empty()){
        // Run inline
        for(auto d:deps){ auto j=m_impl->FindJob(d); if(j) while(!j->done) std::this_thread::yield(); }
        fn(); job->done=true; m_impl->stats.completed++;
        m_impl->jobs.push_back(job);
        return job->handle;
    }
    std::unique_lock<std::mutex> lk(m_impl->mtx);
    m_impl->jobs.push_back(job);
    // Insert by priority
    auto it=m_impl->queue.begin();
    while(it!=m_impl->queue.end()&&(uint8_t)(*it)->priority>=(uint8_t)priority) ++it;
    m_impl->queue.insert(it,job);
    m_impl->cv.notify_one();
    return job->handle;
}

JobHandle JobSystem::ParallelFor(uint32_t count, ParallelFn fn, JobPriority priority){
    std::vector<JobHandle> handles;
    uint32_t threads=m_impl->workers.empty()?1:(uint32_t)m_impl->workers.size();
    uint32_t batch=std::max(1u,(count+threads-1)/threads);
    for(uint32_t start=0;start<count;start+=batch){
        uint32_t end=std::min(start+batch,count);
        handles.push_back(Schedule([fn,start,end](){ for(uint32_t i=start;i<end;i++) fn(i); }, priority));
    }
    // Return a fence job
    return ScheduleAfter([]{}, handles, priority);
}

void JobSystem::Wait(JobHandle h){
    auto job=m_impl->FindJob(h);
    if(!job) return;
    while(!job->done) { m_impl->cv.notify_all(); std::this_thread::yield(); }
}
void JobSystem::WaitAll(){
    while(true){
        bool allDone=true;
        {std::unique_lock<std::mutex> lk(m_impl->mtx); allDone=m_impl->queue.empty();}
        if(allDone) break;
        std::this_thread::yield();
    }
}
bool JobSystem::IsComplete(JobHandle h) const {
    auto job=m_impl->FindJob(h); return job?job->done.load():true;
}
uint32_t JobSystem::ThreadCount() const { return (uint32_t)m_impl->workers.size(); }
JobStats JobSystem::GetStats()    const { return m_impl->stats; }
void     JobSystem::ResetStats()        { m_impl->stats={}; }
void     JobSystem::SetFallbackSingleThread(bool e){ m_impl->singleThread=e; }

} // namespace Core
