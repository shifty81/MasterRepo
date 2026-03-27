#include "Runtime/Gameplay/Checkpoint/CheckpointSystem/CheckpointSystem.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct CPData {
    uint32_t    id;
    CPVec3      pos{};
    std::string label;
    bool        activated{false};
};

struct CheckpointSystem::Impl {
    uint32_t activeId{UINT32_MAX};
    std::vector<CPData>                           checkpoints;
    std::unordered_map<std::string,std::vector<uint8_t>> snapshots;
    std::function<void(uint32_t)>              onActivate;
    std::function<void(uint32_t,CPVec3)>       onRespawn;

    CPData* Find(uint32_t id){
        for(auto& c:checkpoints) if(c.id==id) return &c; return nullptr;
    }
    const CPData* Find(uint32_t id) const {
        for(auto& c:checkpoints) if(c.id==id) return &c; return nullptr;
    }
};

CheckpointSystem::CheckpointSystem(): m_impl(new Impl){}
CheckpointSystem::~CheckpointSystem(){ Shutdown(); delete m_impl; }
void CheckpointSystem::Init(){}
void CheckpointSystem::Shutdown(){ m_impl->checkpoints.clear(); m_impl->snapshots.clear(); }
void CheckpointSystem::Reset(){ Shutdown(); m_impl->activeId=UINT32_MAX; }

void CheckpointSystem::RegisterCheckpoint(uint32_t id, CPVec3 pos, const std::string& label){
    for(auto& c:m_impl->checkpoints) if(c.id==id){ c.pos=pos; c.label=label; return; }
    CPData cp; cp.id=id; cp.pos=pos; cp.label=label;
    m_impl->checkpoints.push_back(cp);
}

void CheckpointSystem::ActivateCheckpoint(uint32_t id){
    auto* cp=m_impl->Find(id); if(!cp) return;
    cp->activated=true;
    m_impl->activeId=id;
    if(m_impl->onActivate) m_impl->onActivate(id);
}

void CheckpointSystem::SaveSnapshot(const std::string& tag, const uint8_t* blob, uint32_t size){
    m_impl->snapshots[tag].assign(blob,blob+size);
}
bool CheckpointSystem::LoadSnapshot(const std::string& tag, std::vector<uint8_t>& out) const {
    auto it=m_impl->snapshots.find(tag);
    if(it==m_impl->snapshots.end()) return false;
    out=it->second; return true;
}
void CheckpointSystem::ClearSnapshots(){ m_impl->snapshots.clear(); }

uint32_t CheckpointSystem::GetActiveCheckpointId() const { return m_impl->activeId; }
CPVec3 CheckpointSystem::GetActiveCheckpointPos() const {
    auto* cp=m_impl->Find(m_impl->activeId);
    return cp?cp->pos:CPVec3{0,0,0};
}
CPVec3 CheckpointSystem::GetCheckpointPos(uint32_t id) const {
    auto* cp=m_impl->Find(id); return cp?cp->pos:CPVec3{0,0,0};
}
std::string CheckpointSystem::GetCheckpointLabel(uint32_t id) const {
    auto* cp=m_impl->Find(id); return cp?cp->label:"";
}
uint32_t CheckpointSystem::GetCheckpointCount() const {
    return (uint32_t)m_impl->checkpoints.size();
}
bool CheckpointSystem::IsActivated(uint32_t id) const {
    auto* cp=m_impl->Find(id); return cp&&cp->activated;
}

void CheckpointSystem::RespawnAt(uint32_t agentId){
    CPVec3 pos=GetActiveCheckpointPos();
    if(m_impl->onRespawn) m_impl->onRespawn(agentId,pos);
}

void CheckpointSystem::SetOnActivate(std::function<void(uint32_t)> cb){ m_impl->onActivate=cb; }
void CheckpointSystem::SetOnRespawn (std::function<void(uint32_t,CPVec3)> cb){ m_impl->onRespawn=cb; }

} // namespace Runtime
