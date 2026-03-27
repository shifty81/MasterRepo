#include "Core/Profiler/MemoryProfiler/MemoryProfiler.h"
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Core {

struct CategoryStats {
    size_t   current{0};
    size_t   peak   {0};
    uint32_t count  {0};
    size_t   thresholdBytes{0};
    std::function<void(size_t)> onThreshold;
    bool     thresholdFired{false};
};

struct MemoryProfiler::Impl {
    bool                                        tracking{true};
    size_t                                      total{0}, peak{0};
    uint32_t                                    allocCount{0};
    uint64_t                                    frame{0};
    std::unordered_map<void*,AllocRecord>       live;
    std::unordered_map<std::string,CategoryStats> cats;
    std::unordered_map<uint32_t,std::vector<AllocRecord>> snapshots;

    void CheckThreshold(const std::string& cat){
        auto& cs=cats[cat];
        if(cs.thresholdBytes>0&&cs.current>=cs.thresholdBytes&&!cs.thresholdFired){
            if(cs.onThreshold) cs.onThreshold(cs.current);
            cs.thresholdFired=true;
        }
        if(cs.current<cs.thresholdBytes) cs.thresholdFired=false;
    }
};

MemoryProfiler::MemoryProfiler(): m_impl(new Impl){}
MemoryProfiler::~MemoryProfiler(){ Shutdown(); delete m_impl; }
void MemoryProfiler::Init(){}
void MemoryProfiler::Shutdown(){ m_impl->live.clear(); m_impl->cats.clear(); }
void MemoryProfiler::Reset(){
    m_impl->live.clear(); m_impl->cats.clear();
    m_impl->total=0; m_impl->peak=0; m_impl->allocCount=0;
}

void MemoryProfiler::BeginFrame(){ m_impl->frame++; }
void MemoryProfiler::EndFrame()  {}
void MemoryProfiler::EnableTracking(bool on){ m_impl->tracking=on; }

void MemoryProfiler::TrackAlloc(void* ptr, size_t bytes,
                                  const std::string& cat, const std::string& tag){
    if(!m_impl->tracking||!ptr) return;
    AllocRecord r; r.ptr=ptr; r.size=bytes; r.category=cat; r.tag=tag;
    r.frameIndex=m_impl->frame;
    m_impl->live[ptr]=r;
    m_impl->total+=bytes;
    m_impl->peak=std::max(m_impl->peak,m_impl->total);
    m_impl->allocCount++;
    auto& cs=m_impl->cats[cat];
    cs.current+=bytes; cs.peak=std::max(cs.peak,cs.current); cs.count++;
    m_impl->CheckThreshold(cat);
}
void MemoryProfiler::TrackFree(void* ptr){
    if(!ptr) return;
    auto it=m_impl->live.find(ptr);
    if(it==m_impl->live.end()) return;
    auto& r=it->second;
    m_impl->total-=r.size;
    m_impl->cats[r.category].current-=r.size;
    m_impl->allocCount--;
    m_impl->live.erase(it);
}

size_t   MemoryProfiler::GetTotalAllocated() const { return m_impl->total; }
size_t   MemoryProfiler::GetPeakAllocated () const { return m_impl->peak; }
uint32_t MemoryProfiler::GetAllocCount    () const { return m_impl->allocCount; }

size_t MemoryProfiler::GetAllocatedByCategory(const std::string& cat) const {
    auto it=m_impl->cats.find(cat); return it!=m_impl->cats.end()?it->second.current:0;
}
size_t MemoryProfiler::GetPeakByCategory(const std::string& cat) const {
    auto it=m_impl->cats.find(cat); return it!=m_impl->cats.end()?it->second.peak:0;
}
uint32_t MemoryProfiler::GetCategories(std::vector<std::string>& out) const {
    out.clear(); for(auto& [k,_]:m_impl->cats) out.push_back(k);
    return (uint32_t)out.size();
}

void MemoryProfiler::TakeSnapshot(uint32_t snapId){
    std::vector<AllocRecord> snap;
    for(auto& [ptr,r]:m_impl->live) snap.push_back(r);
    m_impl->snapshots[snapId]=snap;
}
uint32_t MemoryProfiler::DiffSnapshots(uint32_t snapA, uint32_t snapB,
                                         std::vector<AllocRecord>& added,
                                         std::vector<AllocRecord>& removed) const {
    added.clear(); removed.clear();
    auto itA=m_impl->snapshots.find(snapA);
    auto itB=m_impl->snapshots.find(snapB);
    if(itA==m_impl->snapshots.end()||itB==m_impl->snapshots.end()) return 0;
    std::unordered_set<void*> setA, setB;
    for(auto& r:itA->second) setA.insert(r.ptr);
    for(auto& r:itB->second) setB.insert(r.ptr);
    for(auto& r:itB->second) if(!setA.count(r.ptr)) added.push_back(r);
    for(auto& r:itA->second) if(!setB.count(r.ptr)) removed.push_back(r);
    return (uint32_t)(added.size()+removed.size());
}

uint32_t MemoryProfiler::PrintReport(std::string& out) const {
    std::ostringstream ss;
    ss<<"=== MemoryProfiler Report ===\n";
    ss<<"Total: "<<m_impl->total<<" bytes | Peak: "<<m_impl->peak<<" | Allocs: "<<m_impl->allocCount<<"\n";
    ss<<"--- Categories ---\n";
    for(auto& [cat,cs]:m_impl->cats)
        ss<<"  "<<cat<<": "<<cs.current<<" bytes (peak "<<cs.peak<<", count "<<cs.count<<")\n";
    out=ss.str();
    uint32_t lines=0; for(char c:out) if(c=='\n') lines++;
    return lines;
}

void MemoryProfiler::SetOnThreshold(const std::string& cat, size_t bytes,
                                     std::function<void(size_t)> cb){
    auto& cs=m_impl->cats[cat];
    cs.thresholdBytes=bytes; cs.onThreshold=cb;
}

} // namespace Core
