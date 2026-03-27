#include "Engine/Render/RenderGraph/RenderGraph/RenderGraph.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Engine {

struct RGPass {
    uint32_t id;
    std::string name;
    std::function<void(uint32_t)> setupFn;
    std::function<void(uint32_t)> executeFn;
    std::vector<uint32_t> reads;
    std::vector<uint32_t> writes;
    std::vector<uint32_t> depsBefore; ///< must run before this pass
};

struct RGResource {
    uint32_t id;
    std::string name;
    RGResourceDesc desc;
    uint32_t firstPass{UINT32_MAX};
    uint32_t lastPass {0};
};

struct RenderGraph::Impl {
    uint32_t nextPassId{1};
    uint32_t nextResId {1};
    std::vector<RGPass>     passes;
    std::vector<RGResource> resources;
    std::vector<uint32_t>   sortedOrder;
    bool compiled{false};
    std::function<void(const RGBarrierEvent&)> onBarrier;

    RGPass*     FindPass(uint32_t id){ for(auto& p:passes) if(p.id==id) return &p; return nullptr; }
    RGResource* FindRes (uint32_t id){ for(auto& r:resources) if(r.id==id) return &r; return nullptr; }
    const RGResource* FindRes(uint32_t id) const {
        for(auto& r:resources) if(r.id==id) return &r;
        return nullptr;
    }
};

RenderGraph::RenderGraph(): m_impl(new Impl){}
RenderGraph::~RenderGraph(){ Shutdown(); delete m_impl; }
void RenderGraph::Init(){}
void RenderGraph::Shutdown(){ m_impl->passes.clear(); m_impl->resources.clear(); m_impl->sortedOrder.clear(); }
void RenderGraph::Reset(){ Shutdown(); m_impl->nextPassId=1; m_impl->nextResId=1; m_impl->compiled=false; }

uint32_t RenderGraph::AddResource(const std::string& name, const RGResourceDesc& desc){
    RGResource r; r.id=m_impl->nextResId++; r.name=name; r.desc=desc;
    m_impl->resources.push_back(r); return r.id;
}
uint32_t RenderGraph::GetResourceId(const std::string& name) const {
    for(auto& r:m_impl->resources) if(r.name==name) return r.id;
    return 0;
}

uint32_t RenderGraph::AddPass(const std::string& name,
                               std::function<void(uint32_t)> setup,
                               std::function<void(uint32_t)> execute){
    RGPass p; p.id=m_impl->nextPassId++; p.name=name;
    p.setupFn=setup; p.executeFn=execute;
    if(setup) setup(p.id);
    m_impl->passes.push_back(p);
    m_impl->compiled=false;
    return p.id;
}
void RenderGraph::DeclareRead(uint32_t passId, uint32_t resId){
    auto* p=m_impl->FindPass(passId); if(p) p->reads.push_back(resId);
    m_impl->compiled=false;
}
void RenderGraph::DeclareWrite(uint32_t passId, uint32_t resId){
    auto* p=m_impl->FindPass(passId); if(p) p->writes.push_back(resId);
    m_impl->compiled=false;
}
void RenderGraph::AddDependency(uint32_t afterPassId, uint32_t beforePassId){
    auto* p=m_impl->FindPass(afterPassId);
    if(p) p->depsBefore.push_back(beforePassId);
    m_impl->compiled=false;
}

bool RenderGraph::Compile(){
    // Build adjacency: passId → set of passes that must run before it
    std::unordered_map<uint32_t,std::unordered_set<uint32_t>> deps;
    for(auto& p:m_impl->passes){
        deps[p.id]; // ensure entry
        for(auto before:p.depsBefore) deps[p.id].insert(before);
        // If a pass reads a resource, it must run after any pass that writes it
        for(auto rid:p.reads){
            for(auto& q:m_impl->passes){
                if(q.id==p.id) continue;
                for(auto w:q.writes) if(w==rid) deps[p.id].insert(q.id);
            }
        }
    }
    // Kahn's topological sort
    std::unordered_map<uint32_t,uint32_t> inDeg;
    for(auto& p:m_impl->passes) inDeg[p.id]=(uint32_t)deps[p.id].size();
    std::vector<uint32_t> queue;
    for(auto& [id,d]:inDeg) if(d==0) queue.push_back(id);
    m_impl->sortedOrder.clear();
    while(!queue.empty()){
        uint32_t cur=queue.back(); queue.pop_back();
        m_impl->sortedOrder.push_back(cur);
        for(auto& p:m_impl->passes){
            if(deps[p.id].erase(cur)){
                if(--inDeg[p.id]==0) queue.push_back(p.id);
            }
        }
    }
    if(m_impl->sortedOrder.size()!=m_impl->passes.size()) return false; // cycle
    // Compute resource lifetimes
    for(size_t order=0;order<m_impl->sortedOrder.size();order++){
        uint32_t pid=m_impl->sortedOrder[order];
        auto* pass=m_impl->FindPass(pid);
        if(!pass) continue;
        auto touch=[&](uint32_t rid){
            auto* r=m_impl->FindRes(rid); if(!r) return;
            if(order<r->firstPass) r->firstPass=(uint32_t)order;
            if(order>r->lastPass)  r->lastPass =(uint32_t)order;
        };
        for(auto rid:pass->reads)  touch(rid);
        for(auto rid:pass->writes) touch(rid);
    }
    m_impl->compiled=true;
    return true;
}

void RenderGraph::Execute(){
    if(!m_impl->compiled) Compile();
    for(uint32_t pid:m_impl->sortedOrder){
        auto* p=m_impl->FindPass(pid); if(!p) continue;
        // Emit barriers: reads need read barrier after write, writes need write barrier
        if(m_impl->onBarrier){
            for(auto rid:p->reads){
                RGBarrierEvent ev; ev.resourceId=rid; ev.srcPassId=0;
                ev.dstPassId=pid; ev.toWrite=false;
                m_impl->onBarrier(ev);
            }
            for(auto rid:p->writes){
                RGBarrierEvent ev; ev.resourceId=rid; ev.srcPassId=0;
                ev.dstPassId=pid; ev.toWrite=true;
                m_impl->onBarrier(ev);
            }
        }
        if(p->executeFn) p->executeFn(pid);
    }
}

std::vector<uint32_t> RenderGraph::GetPassOrder() const { return m_impl->sortedOrder; }
void RenderGraph::GetResourceLifetime(uint32_t id, uint32_t& first, uint32_t& last) const {
    auto* r=m_impl->FindRes(id);
    first=r?r->firstPass:0; last=r?r->lastPass:0;
}
uint32_t RenderGraph::GetPassCount()     const { return (uint32_t)m_impl->passes.size(); }
uint32_t RenderGraph::GetResourceCount() const { return (uint32_t)m_impl->resources.size(); }
bool     RenderGraph::IsCompiled()       const { return m_impl->compiled; }
void     RenderGraph::SetOnBarrier(std::function<void(const RGBarrierEvent&)> cb){ m_impl->onBarrier=cb; }

} // namespace Engine
