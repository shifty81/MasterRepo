#include "Runtime/Gameplay/ObjectiveSystem/ObjectiveSystem.h"
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct ObjectiveSystem::Impl {
    std::unordered_map<std::string,ObjectiveInstance> objectives;

    std::function<void(const std::string&)>                           onActivated;
    std::function<void(const std::string&)>                           onCompleted;
    std::function<void(const std::string&)>                           onFailed;
    std::function<void(const std::string&,int32_t,int32_t)>           onProgress;

    void TryComplete(const std::string& id){
        auto it=objectives.find(id); if(it==objectives.end()) return;
        auto& obj=it->second;
        if(obj.state!=ObjectiveState::Active) return;
        if(obj.progress>=obj.desc.targetCount){
            obj.state=ObjectiveState::Completed;
            if(onCompleted) onCompleted(id);
            // Chain
            if(!obj.desc.nextObjectiveId.empty()){
                auto nit=objectives.find(obj.desc.nextObjectiveId);
                if(nit!=objectives.end()&&nit->second.state==ObjectiveState::Inactive){
                    nit->second.state=ObjectiveState::Active;
                    if(onActivated) onActivated(obj.desc.nextObjectiveId);
                }
            }
        }
    }
};

ObjectiveSystem::ObjectiveSystem() : m_impl(new Impl()) {}
ObjectiveSystem::~ObjectiveSystem() { delete m_impl; }
void ObjectiveSystem::Init()     {}
void ObjectiveSystem::Shutdown() { m_impl->objectives.clear(); }

void ObjectiveSystem::RegisterObjective(const ObjectiveDesc& desc){
    ObjectiveInstance inst; inst.desc=desc;
    m_impl->objectives[desc.id]=inst;
}
void ObjectiveSystem::UnregisterObjective(const std::string& id){ m_impl->objectives.erase(id); }
bool ObjectiveSystem::HasObjective(const std::string& id) const{ return m_impl->objectives.count(id)>0; }
ObjectiveInstance ObjectiveSystem::GetObjective(const std::string& id) const{
    auto it=m_impl->objectives.find(id); return it!=m_impl->objectives.end()?it->second:ObjectiveInstance{};
}

std::vector<ObjectiveInstance> ObjectiveSystem::AllObjectives() const{
    std::vector<ObjectiveInstance> out; for(auto&[k,v]:m_impl->objectives) out.push_back(v); return out;
}
std::vector<ObjectiveInstance> ObjectiveSystem::ActiveObjectives() const{
    std::vector<ObjectiveInstance> out;
    for(auto&[k,v]:m_impl->objectives) if(v.state==ObjectiveState::Active) out.push_back(v);
    return out;
}

void ObjectiveSystem::ActivateObjective(const std::string& id){
    auto it=m_impl->objectives.find(id); if(it==m_impl->objectives.end()) return;
    if(it->second.state==ObjectiveState::Inactive){
        it->second.state=ObjectiveState::Active;
        if(m_impl->onActivated) m_impl->onActivated(id);
    }
}
void ObjectiveSystem::CompleteObjective(const std::string& id){
    auto it=m_impl->objectives.find(id); if(it==m_impl->objectives.end()) return;
    it->second.state=ObjectiveState::Completed;
    if(m_impl->onCompleted) m_impl->onCompleted(id);
}
void ObjectiveSystem::FailObjective(const std::string& id){
    auto it=m_impl->objectives.find(id); if(it==m_impl->objectives.end()) return;
    it->second.state=ObjectiveState::Failed;
    if(m_impl->onFailed) m_impl->onFailed(id);
}
void ObjectiveSystem::ResetObjective(const std::string& id){
    auto it=m_impl->objectives.find(id); if(it==m_impl->objectives.end()) return;
    it->second.state=ObjectiveState::Inactive;
    it->second.progress=0; it->second.elapsed=0.f;
}

void ObjectiveSystem::IncrementProgress(const std::string& id, int32_t amount){
    auto it=m_impl->objectives.find(id); if(it==m_impl->objectives.end()) return;
    if(it->second.state!=ObjectiveState::Active) return;
    it->second.progress=std::min(it->second.progress+amount, it->second.desc.targetCount);
    if(m_impl->onProgress) m_impl->onProgress(id,it->second.progress,it->second.desc.targetCount);
    m_impl->TryComplete(id);
}
void ObjectiveSystem::SetProgress(const std::string& id, int32_t val){
    auto it=m_impl->objectives.find(id); if(it==m_impl->objectives.end()) return;
    it->second.progress=std::max(0,std::min(val,it->second.desc.targetCount));
    if(m_impl->onProgress) m_impl->onProgress(id,it->second.progress,it->second.desc.targetCount);
    m_impl->TryComplete(id);
}

bool ObjectiveSystem::IsCompleted(const std::string& id) const{
    auto it=m_impl->objectives.find(id); return it!=m_impl->objectives.end()&&it->second.state==ObjectiveState::Completed;
}
bool ObjectiveSystem::IsFailed(const std::string& id) const{
    auto it=m_impl->objectives.find(id); return it!=m_impl->objectives.end()&&it->second.state==ObjectiveState::Failed;
}
bool ObjectiveSystem::IsActive(const std::string& id) const{
    auto it=m_impl->objectives.find(id); return it!=m_impl->objectives.end()&&it->second.state==ObjectiveState::Active;
}

void ObjectiveSystem::Update(float dt){
    for(auto&[id,obj]:m_impl->objectives){
        if(obj.state!=ObjectiveState::Active) continue;
        if(obj.desc.timeLimit>0.f){
            obj.elapsed+=dt;
            if(obj.elapsed>=obj.desc.timeLimit) FailObjective(id);
        }
    }
}

bool ObjectiveSystem::SaveState(const std::string& path) const{
    std::ofstream f(path); if(!f.is_open()) return false;
    for(auto&[id,obj]:m_impl->objectives)
        f<<id<<":"<<(int)obj.state<<":"<<obj.progress<<"\n";
    return true;
}
bool ObjectiveSystem::LoadState(const std::string& path){
    std::ifstream f(path); return f.is_open();
}

void ObjectiveSystem::OnActivated(std::function<void(const std::string&)> cb)     { m_impl->onActivated=std::move(cb); }
void ObjectiveSystem::OnCompleted(std::function<void(const std::string&)> cb)     { m_impl->onCompleted=std::move(cb); }
void ObjectiveSystem::OnFailed(std::function<void(const std::string&)> cb)        { m_impl->onFailed=std::move(cb); }
void ObjectiveSystem::OnProgressChanged(std::function<void(const std::string&,int32_t,int32_t)> cb){ m_impl->onProgress=std::move(cb); }

} // namespace Runtime
