#include "Runtime/Combat/AbilityQueue/AbilityQueue/AbilityQueue.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct AbilityEntry {
    uint32_t abilityId;
    int32_t  effectivePriority;
};

struct AgentAbilityState {
    std::vector<AbilityEntry> queue;
    // Casting state
    uint32_t castingAbilityId{0};
    float    castTimeRemaining{0};
    bool     isCasting{false};
    // Cooldowns: abilityId → remaining
    std::unordered_map<uint32_t,float> cooldowns;
};

struct AbilityQueue::Impl {
    std::unordered_map<uint32_t,AbilityDef>          defs;
    std::unordered_map<uint32_t,int32_t>             priorityBoosts;
    std::unordered_map<uint32_t,AgentAbilityState>   agents;
    uint32_t maxQueueLen{8};

    std::function<void(uint32_t,uint32_t)> onActivate;
    std::function<void(uint32_t,uint32_t)> onInterrupt;
    std::function<void(uint32_t,uint32_t)> onCooldownEnd;

    AbilityDef* FindDef(uint32_t id){
        auto it=defs.find(id); return it!=defs.end()?&it->second:nullptr;
    }
    AgentAbilityState& GetAgent(uint32_t id){ return agents[id]; }
};

AbilityQueue::AbilityQueue(): m_impl(new Impl){}
AbilityQueue::~AbilityQueue(){ Shutdown(); delete m_impl; }
void AbilityQueue::Init(){}
void AbilityQueue::Shutdown(){ m_impl->defs.clear(); m_impl->agents.clear(); }
void AbilityQueue::Reset(){ Shutdown(); }

void AbilityQueue::RegisterAbility(const AbilityDef& def){ m_impl->defs[def.id]=def; }
void AbilityQueue::SetPriorityBoost(uint32_t id, int32_t boost){ m_impl->priorityBoosts[id]=boost; }

void AbilityQueue::Enqueue(uint32_t agentId, uint32_t abilityId){
    auto& ag=m_impl->GetAgent(agentId);
    if(ag.queue.size()>=m_impl->maxQueueLen) return;
    auto* def=m_impl->FindDef(abilityId); if(!def) return;
    int32_t boost=0;
    auto it=m_impl->priorityBoosts.find(abilityId);
    if(it!=m_impl->priorityBoosts.end()) boost=it->second;
    AbilityEntry e; e.abilityId=abilityId; e.effectivePriority=def->priority+boost;
    ag.queue.push_back(e);
    // Sort descending by priority
    std::stable_sort(ag.queue.begin(),ag.queue.end(),
        [](const AbilityEntry& a,const AbilityEntry& b){ return a.effectivePriority>b.effectivePriority; });
}
void AbilityQueue::ClearQueue(uint32_t agentId){ m_impl->GetAgent(agentId).queue.clear(); }
void AbilityQueue::Interrupt  (uint32_t agentId){
    auto& ag=m_impl->GetAgent(agentId);
    if(ag.isCasting){
        uint32_t cid=ag.castingAbilityId;
        ag.isCasting=false; ag.castTimeRemaining=0;
        if(m_impl->onInterrupt) m_impl->onInterrupt(agentId,cid);
    }
    ag.queue.clear();
}
void AbilityQueue::ForceActivate(uint32_t agentId, uint32_t abilityId){
    if(m_impl->onActivate) m_impl->onActivate(agentId,abilityId);
    auto* def=m_impl->FindDef(abilityId);
    if(def&&def->cooldown>0) m_impl->GetAgent(agentId).cooldowns[abilityId]=def->cooldown;
}

void AbilityQueue::Tick(float dt){
    for(auto& [agentId,ag]:m_impl->agents){
        // Advance cooldowns
        std::vector<uint32_t> endedCDs;
        for(auto& [aid,cd]:ag.cooldowns){
            cd-=dt;
            if(cd<=0){ endedCDs.push_back(aid); }
        }
        for(auto aid:endedCDs){
            ag.cooldowns.erase(aid);
            if(m_impl->onCooldownEnd) m_impl->onCooldownEnd(agentId,aid);
        }

        // Casting
        if(ag.isCasting){
            ag.castTimeRemaining-=dt;
            if(ag.castTimeRemaining<=0){
                ag.isCasting=false;
                if(m_impl->onActivate) m_impl->onActivate(agentId,ag.castingAbilityId);
                auto* def=m_impl->FindDef(ag.castingAbilityId);
                if(def&&def->cooldown>0) ag.cooldowns[ag.castingAbilityId]=def->cooldown;
            }
            continue;
        }

        // Pop next queued
        if(!ag.queue.empty()){
            AbilityEntry top=ag.queue.front();
            // Check cooldown
            auto cdit=ag.cooldowns.find(top.abilityId);
            if(cdit!=ag.cooldowns.end()&&cdit->second>0){
                ag.queue.erase(ag.queue.begin()); continue;
            }
            ag.queue.erase(ag.queue.begin());
            auto* def=m_impl->FindDef(top.abilityId);
            if(def&&def->castTime>0){
                ag.isCasting=true; ag.castingAbilityId=top.abilityId;
                ag.castTimeRemaining=def->castTime;
            } else {
                if(m_impl->onActivate) m_impl->onActivate(agentId,top.abilityId);
                if(def&&def->cooldown>0) ag.cooldowns[top.abilityId]=def->cooldown;
            }
        }
    }
}

bool  AbilityQueue::IsOnCooldown(uint32_t agentId, uint32_t abilityId) const {
    auto ait=m_impl->agents.find(agentId); if(ait==m_impl->agents.end()) return false;
    auto& ag=ait->second;
    auto cit=ag.cooldowns.find(abilityId);
    return cit!=ag.cooldowns.end()&&cit->second>0;
}
float AbilityQueue::GetCooldownRemaining(uint32_t agentId, uint32_t abilityId) const {
    auto ait=m_impl->agents.find(agentId); if(ait==m_impl->agents.end()) return 0;
    auto& ag=ait->second;
    auto cit=ag.cooldowns.find(abilityId);
    return cit!=ag.cooldowns.end()?std::max(0.f,cit->second):0;
}
uint32_t AbilityQueue::GetQueueLength(uint32_t agentId) const {
    auto ait=m_impl->agents.find(agentId);
    return ait!=m_impl->agents.end()?(uint32_t)ait->second.queue.size():0;
}
void AbilityQueue::SetMaxQueueLength(uint32_t n){ m_impl->maxQueueLen=n; }

void AbilityQueue::SetOnActivate  (std::function<void(uint32_t,uint32_t)> cb){ m_impl->onActivate=cb; }
void AbilityQueue::SetOnInterrupt (std::function<void(uint32_t,uint32_t)> cb){ m_impl->onInterrupt=cb; }
void AbilityQueue::SetOnCooldownEnd(std::function<void(uint32_t,uint32_t)> cb){ m_impl->onCooldownEnd=cb; }

} // namespace Runtime
