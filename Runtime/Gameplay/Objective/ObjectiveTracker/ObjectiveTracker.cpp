#include "Runtime/Gameplay/Objective/ObjectiveTracker/ObjectiveTracker.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct ObjData {
    ObjectiveDef  def;
    ObjectiveState state{ObjectiveState::Inactive};
    float          progress{0};
    std::function<void(float,float)> onProgress;
    std::function<void()>            onComplete;
    std::function<void()>            onFail;
};

struct Chain { uint32_t from, to; };

struct ObjectiveTracker::Impl {
    std::unordered_map<uint32_t,ObjData> objectives;
    std::vector<Chain>                   chains;

    ObjData* Find(uint32_t id){
        auto it=objectives.find(id); return it!=objectives.end()?&it->second:nullptr;
    }
    void TryActivateChain(uint32_t fromId){
        for(auto& c:chains) if(c.from==fromId){
            auto* to=Find(c.to);
            if(to&&to->state==ObjectiveState::Inactive)
                to->state=ObjectiveState::Active;
        }
    }
};

ObjectiveTracker::ObjectiveTracker(): m_impl(new Impl){}
ObjectiveTracker::~ObjectiveTracker(){ Shutdown(); delete m_impl; }
void ObjectiveTracker::Init(){}
void ObjectiveTracker::Shutdown(){ m_impl->objectives.clear(); m_impl->chains.clear(); }
void ObjectiveTracker::Reset(){ Shutdown(); }

bool ObjectiveTracker::RegisterObjective(const ObjectiveDef& def){
    if(m_impl->objectives.count(def.id)) return false;
    ObjData od; od.def=def;
    m_impl->objectives[def.id]=od;
    return true;
}
void ObjectiveTracker::Activate  (uint32_t id){
    auto* o=m_impl->Find(id); if(o&&o->state==ObjectiveState::Inactive) o->state=ObjectiveState::Active;
}
void ObjectiveTracker::Deactivate(uint32_t id){
    auto* o=m_impl->Find(id); if(o) o->state=ObjectiveState::Inactive;
}
void ObjectiveTracker::SetProgress(uint32_t id, float cur){
    auto* o=m_impl->Find(id); if(!o||o->state!=ObjectiveState::Active) return;
    o->progress=std::min(cur, o->def.required);
    if(o->onProgress) o->onProgress(o->progress, o->def.required);
    if(o->progress>=o->def.required) CompleteObjective(id);
}
void ObjectiveTracker::IncrementProgress(uint32_t id, float delta){
    auto* o=m_impl->Find(id); if(!o) return;
    SetProgress(id, o->progress+delta);
}
float ObjectiveTracker::GetProgress(uint32_t id) const {
    auto it=m_impl->objectives.find(id); return it!=m_impl->objectives.end()?it->second.progress:0;
}
float ObjectiveTracker::GetRequired(uint32_t id) const {
    auto it=m_impl->objectives.find(id); return it!=m_impl->objectives.end()?it->second.def.required:0;
}
bool ObjectiveTracker::IsComplete(uint32_t id) const {
    return GetState(id)==ObjectiveState::Complete;
}
void ObjectiveTracker::CompleteObjective(uint32_t id){
    auto* o=m_impl->Find(id); if(!o) return;
    o->state=ObjectiveState::Complete;
    o->progress=o->def.required;
    if(o->onComplete) o->onComplete();
    m_impl->TryActivateChain(id);
}
void ObjectiveTracker::FailObjective(uint32_t id){
    auto* o=m_impl->Find(id); if(!o) return;
    o->state=ObjectiveState::Failed;
    if(o->onFail) o->onFail();
}
ObjectiveState ObjectiveTracker::GetState(uint32_t id) const {
    auto it=m_impl->objectives.find(id);
    return it!=m_impl->objectives.end()?it->second.state:ObjectiveState::Inactive;
}
void ObjectiveTracker::AddChain(uint32_t from, uint32_t to){
    m_impl->chains.push_back({from,to});
}
uint32_t ObjectiveTracker::GetActiveObjectiveIds(std::vector<uint32_t>& out) const {
    out.clear();
    for(auto& [id,o]:m_impl->objectives)
        if(o.state==ObjectiveState::Active) out.push_back(id);
    return (uint32_t)out.size();
}
void ObjectiveTracker::SetOnProgress(uint32_t id,std::function<void(float,float)> cb){
    auto* o=m_impl->Find(id); if(o) o->onProgress=cb;
}
void ObjectiveTracker::SetOnComplete(uint32_t id,std::function<void()> cb){
    auto* o=m_impl->Find(id); if(o) o->onComplete=cb;
}
void ObjectiveTracker::SetOnFail(uint32_t id,std::function<void()> cb){
    auto* o=m_impl->Find(id); if(o) o->onFail=cb;
}

} // namespace Runtime
