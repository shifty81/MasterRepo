#include "Runtime/Gameplay/Skills/SkillSystem/SkillSystem.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct SkillPrereq { uint32_t skillId; uint32_t minLevel; };

struct SkillEntry {
    SkillDesc desc;
    std::vector<SkillPrereq> prereqs;
    std::function<void(uint32_t, uint32_t)> onActivate;
};

struct AgentSkillState {
    uint32_t level{0};
    bool     unlocked{false};
    float    cooldownRemaining{0};
};

struct AgentState {
    float    resource{100.f};
    float    maxResource{100.f};
    float    regenRate{0.f};
    std::unordered_map<uint32_t, AgentSkillState> skills;
};

struct SkillSystem::Impl {
    std::vector<SkillEntry> skills;
    std::unordered_map<uint32_t, AgentState> agents;

    SkillEntry* FindSkill(uint32_t id){
        for(auto& s:skills) if(s.desc.id==id) return &s;
        return nullptr;
    }
    const SkillEntry* FindSkill(uint32_t id) const {
        for(auto& s:skills) if(s.desc.id==id) return &s;
        return nullptr;
    }
    AgentState& GetAgent(uint32_t id){ return agents[id]; }
};

SkillSystem::SkillSystem(): m_impl(new Impl){}
SkillSystem::~SkillSystem(){ Shutdown(); delete m_impl; }
void SkillSystem::Init(){}
void SkillSystem::Shutdown(){ m_impl->skills.clear(); m_impl->agents.clear(); }
void SkillSystem::Reset(){ Shutdown(); }

void SkillSystem::Tick(float dt){
    for(auto& [aid, ag]:m_impl->agents){
        // Regen resource
        if(ag.regenRate>0){
            ag.resource=std::min(ag.maxResource, ag.resource+ag.regenRate*dt);
        }
        // Advance cooldowns
        for(auto& [sid, ss]:ag.skills){
            if(ss.cooldownRemaining>0) ss.cooldownRemaining=std::max(0.f,ss.cooldownRemaining-dt);
        }
    }
}

void SkillSystem::RegisterSkill(const SkillDesc& desc){
    SkillEntry e; e.desc=desc; m_impl->skills.push_back(e);
}
uint32_t SkillSystem::GetSkillId(const std::string& name) const {
    for(auto& s:m_impl->skills) if(s.desc.name==name) return s.desc.id;
    return 0;
}

bool SkillSystem::UnlockSkill(uint32_t agentId, uint32_t skillId){
    if(!CanUnlock(agentId,skillId)) return false;
    auto& ag=m_impl->GetAgent(agentId);
    ag.skills[skillId].unlocked=true;
    if(ag.skills[skillId].level==0) ag.skills[skillId].level=1;
    return true;
}

bool SkillSystem::LevelUp(uint32_t agentId, uint32_t skillId){
    auto* se=m_impl->FindSkill(skillId); if(!se) return false;
    auto& ag=m_impl->GetAgent(agentId);
    auto& ss=ag.skills[skillId];
    if(!ss.unlocked||ss.level>=se->desc.maxLevel) return false;
    ss.level++;
    return true;
}

bool SkillSystem::Activate(uint32_t agentId, uint32_t skillId){
    auto* se=m_impl->FindSkill(skillId); if(!se) return false;
    auto& ag=m_impl->GetAgent(agentId);
    auto& ss=ag.skills[skillId];
    if(!ss.unlocked||ss.level==0) return false;
    if(ss.cooldownRemaining>0) return false;
    float cost=se->desc.baseCost+se->desc.costPerLevel*(ss.level-1);
    if(ag.resource<cost) return false;
    ag.resource-=cost;
    ss.cooldownRemaining=se->desc.cooldownSec;
    if(se->onActivate) se->onActivate(agentId,ss.level);
    return true;
}

void  SkillSystem::SetResource    (uint32_t id, float v){ m_impl->GetAgent(id).resource=v; }
float SkillSystem::GetResource    (uint32_t id) const {
    auto it=m_impl->agents.find(id); return it!=m_impl->agents.end()?it->second.resource:0.f;
}
void SkillSystem::SetRegenRate    (uint32_t id, float r){ m_impl->GetAgent(id).regenRate=r; }
void SkillSystem::RegenerateResource(uint32_t id, float dt){
    auto& ag=m_impl->GetAgent(id);
    ag.resource=std::min(ag.maxResource, ag.resource+ag.regenRate*dt);
}
void SkillSystem::SetMaxResource  (uint32_t id, float m){ m_impl->GetAgent(id).maxResource=m; }

bool SkillSystem::IsUnlocked(uint32_t agentId, uint32_t skillId) const {
    auto it=m_impl->agents.find(agentId); if(it==m_impl->agents.end()) return false;
    auto sit=it->second.skills.find(skillId); return sit!=it->second.skills.end()&&sit->second.unlocked;
}
uint32_t SkillSystem::GetLevel(uint32_t agentId, uint32_t skillId) const {
    auto it=m_impl->agents.find(agentId); if(it==m_impl->agents.end()) return 0;
    auto sit=it->second.skills.find(skillId); return sit!=it->second.skills.end()?sit->second.level:0;
}
float SkillSystem::GetCooldownRemaining(uint32_t agentId, uint32_t skillId) const {
    auto it=m_impl->agents.find(agentId); if(it==m_impl->agents.end()) return 0;
    auto sit=it->second.skills.find(skillId);
    return sit!=it->second.skills.end()?sit->second.cooldownRemaining:0.f;
}

bool SkillSystem::CanUnlock(uint32_t agentId, uint32_t skillId) const {
    auto* se=m_impl->FindSkill(skillId); if(!se) return false;
    for(auto& req:se->prereqs){
        if(GetLevel(agentId,req.skillId)<req.minLevel) return false;
    }
    return true;
}

void SkillSystem::SetPrerequisite(uint32_t skillId, uint32_t prereqId, uint32_t prereqLevel){
    auto* se=m_impl->FindSkill(skillId); if(se) se->prereqs.push_back({prereqId,prereqLevel});
}
void SkillSystem::SetOnActivate(uint32_t skillId, std::function<void(uint32_t,uint32_t)> cb){
    auto* se=m_impl->FindSkill(skillId); if(se) se->onActivate=cb;
}

} // namespace Runtime
