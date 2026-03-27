#include "Core/Coroutine/CoroutineScheduler/CoroutineScheduler.h"
#include <algorithm>
#include <vector>

namespace Core {

struct CoroutineEntry {
    CoroutineHandle handle;
    CoroutineFn     fn;
    int32_t         priority;
    std::function<void()> onComplete;
    CoroutineContext ctx;
    bool            alive{true};
};

struct CoroutineScheduler::Impl {
    std::vector<CoroutineEntry> entries;
    uint64_t nextHandle{1};
    uint32_t maxCoroutines{256};
};

CoroutineScheduler::CoroutineScheduler()  : m_impl(new Impl){}
CoroutineScheduler::~CoroutineScheduler() { Shutdown(); delete m_impl; }
void CoroutineScheduler::Init()     {}
void CoroutineScheduler::Shutdown() { m_impl->entries.clear(); }

CoroutineHandle CoroutineScheduler::Start(CoroutineFn fn, int32_t priority,
                                            std::function<void()> onComplete)
{
    if((uint32_t)m_impl->entries.size()>=m_impl->maxCoroutines) return 0;
    CoroutineEntry e; e.handle=m_impl->nextHandle++; e.fn=fn;
    e.priority=priority; e.onComplete=onComplete; e.alive=true;
    m_impl->entries.push_back(e); return e.handle;
}

void CoroutineScheduler::Stop(CoroutineHandle h){
    for(auto& e:m_impl->entries) if(e.handle==h){ e.alive=false; return; }
}
void CoroutineScheduler::StopAll(){
    for(auto& e:m_impl->entries) e.alive=false;
}
bool CoroutineScheduler::IsAlive(CoroutineHandle h) const {
    for(auto& e:m_impl->entries) if(e.handle==h) return e.alive; return false;
}
uint32_t CoroutineScheduler::Count() const {
    uint32_t n=0; for(auto& e:m_impl->entries) if(e.alive) n++; return n;
}
void CoroutineScheduler::SetMaxCoroutines(uint32_t m){ m_impl->maxCoroutines=m; }

void CoroutineScheduler::Tick(float dt)
{
    // Sort by priority
    std::stable_sort(m_impl->entries.begin(),m_impl->entries.end(),
        [](const CoroutineEntry& a, const CoroutineEntry& b){ return a.priority<b.priority; });

    for(auto& e : m_impl->entries){
        if(!e.alive) continue;

        // Check wait conditions
        if(e.ctx.waitRemaining>0.f){ e.ctx.waitRemaining-=dt; continue; }
        if(e.ctx.waitFrames>0){ e.ctx.waitFrames--; continue; }
        if(e.ctx.waitPredicate){ if(!e.ctx.waitPredicate()) continue; e.ctx.waitPredicate={}; }

        // Reset context wait state before calling
        e.ctx.waitRemaining=0.f; e.ctx.waitFrames=0;

        bool stillRunning=e.fn(e.ctx);
        if(!stillRunning){
            e.alive=false;
            if(e.onComplete) e.onComplete();
        }
    }

    // Remove dead entries
    m_impl->entries.erase(
        std::remove_if(m_impl->entries.begin(),m_impl->entries.end(),
            [](const CoroutineEntry& e){ return !e.alive; }),
        m_impl->entries.end());
}

} // namespace Core
