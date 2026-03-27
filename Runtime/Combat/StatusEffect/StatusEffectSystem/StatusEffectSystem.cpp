#include "Runtime/Combat/StatusEffect/StatusEffectSystem/StatusEffectSystem.h"
#include <algorithm>
#include <cstdlib>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct EffectInstance {
    uint32_t effectId;
    uint32_t stacks;
    float    remaining;
    float    tickAccum;
};

struct EntityEffects {
    std::vector<EffectInstance> active;
    std::unordered_map<uint32_t,float> resistance; // effectId → [0,1]
};

struct EffectMeta {
    StatusEffectDef def;
    std::function<void(uint32_t,uint32_t)>        onApply;
    std::function<void(uint32_t,uint32_t,float)>  onTick;
    std::function<void(uint32_t)>                 onExpire;
};

struct StatusEffectSystem::Impl {
    std::unordered_map<uint32_t,EffectMeta>   defs;
    std::unordered_map<uint32_t,EntityEffects> entities;

    EffectInstance* FindInstance(uint32_t entity, uint32_t effectId){
        auto it=entities.find(entity); if(it==entities.end()) return nullptr;
        for(auto& inst:it->second.active) if(inst.effectId==effectId) return &inst;
        return nullptr;
    }
    EffectMeta* FindMeta(uint32_t id){ auto it=defs.find(id); return it!=defs.end()?&it->second:nullptr; }
};

StatusEffectSystem::StatusEffectSystem(): m_impl(new Impl){}
StatusEffectSystem::~StatusEffectSystem(){ Shutdown(); delete m_impl; }
void StatusEffectSystem::Init(){}
void StatusEffectSystem::Shutdown(){ m_impl->defs.clear(); m_impl->entities.clear(); }
void StatusEffectSystem::Reset(){ Shutdown(); }

bool StatusEffectSystem::RegisterEffect(const StatusEffectDef& def){
    if(m_impl->defs.count(def.id)) return false;
    EffectMeta m; m.def=def; m_impl->defs[def.id]=m; return true;
}

uint32_t StatusEffectSystem::Apply(uint32_t entityId, uint32_t effectId, uint32_t stacks){
    auto* meta=m_impl->FindMeta(effectId); if(!meta) return 0;
    // Resistance check
    auto& ent=m_impl->entities[entityId];
    auto rit=ent.resistance.find(effectId);
    if(rit!=ent.resistance.end()){
        float roll=(float)(std::rand()%1000)/1000.f;
        if(roll<rit->second) return 0;
    }
    auto* inst=m_impl->FindInstance(entityId,effectId);
    if(inst){
        uint32_t newStacks=std::min(inst->stacks+stacks,meta->def.maxStacks);
        inst->stacks=newStacks;
        inst->remaining=meta->def.duration; // refresh
    } else {
        EffectInstance ei; ei.effectId=effectId;
        ei.stacks=std::min(stacks,meta->def.maxStacks);
        ei.remaining=meta->def.duration; ei.tickAccum=0;
        ent.active.push_back(ei);
    }
    if(meta->onApply) meta->onApply(entityId,stacks);
    return effectId;
}

void StatusEffectSystem::Remove(uint32_t entityId, uint32_t effectId){
    auto it=m_impl->entities.find(entityId); if(it==m_impl->entities.end()) return;
    auto& v=it->second.active;
    v.erase(std::remove_if(v.begin(),v.end(),[effectId](const EffectInstance& i){return i.effectId==effectId;}),v.end());
}
void StatusEffectSystem::RemoveAll(uint32_t entityId){
    auto it=m_impl->entities.find(entityId); if(it!=m_impl->entities.end()) it->second.active.clear();
}

void StatusEffectSystem::Tick(float dt){
    for(auto& [entityId,ent]:m_impl->entities){
        std::vector<uint32_t> expired;
        for(auto& inst:ent.active){
            auto* meta=m_impl->FindMeta(inst.effectId); if(!meta) continue;
            // Tick callback
            inst.tickAccum+=dt;
            while(inst.tickAccum>=meta->def.tickInterval){
                if(meta->onTick) meta->onTick(entityId,inst.stacks,meta->def.tickInterval);
                inst.tickAccum-=meta->def.tickInterval;
            }
            // Duration
            inst.remaining-=dt;
            if(inst.remaining<=0) expired.push_back(inst.effectId);
        }
        for(auto eid:expired){
            auto* meta=m_impl->FindMeta(eid);
            if(meta&&meta->onExpire) meta->onExpire(entityId);
            Remove(entityId,eid);
        }
    }
}

uint32_t StatusEffectSystem::GetActiveEffects(uint32_t entityId, std::vector<uint32_t>& out) const {
    out.clear(); auto it=m_impl->entities.find(entityId); if(it==m_impl->entities.end()) return 0;
    for(auto& i:it->second.active) out.push_back(i.effectId);
    return (uint32_t)out.size();
}
uint32_t StatusEffectSystem::GetStackCount(uint32_t entityId,uint32_t effectId) const {
    auto* i=m_impl->FindInstance(entityId,effectId); return i?i->stacks:0;
}
float StatusEffectSystem::GetRemainingTime(uint32_t entityId,uint32_t effectId) const {
    auto* i=m_impl->FindInstance(entityId,effectId); return i?i->remaining:0;
}
bool StatusEffectSystem::HasEffect(uint32_t entityId,uint32_t effectId) const {
    return m_impl->FindInstance(entityId,effectId)!=nullptr;
}

void StatusEffectSystem::SetOnApply(uint32_t id,std::function<void(uint32_t,uint32_t)> cb){
    auto* m=m_impl->FindMeta(id); if(m) m->onApply=cb;
}
void StatusEffectSystem::SetOnTick(uint32_t id,std::function<void(uint32_t,uint32_t,float)> cb){
    auto* m=m_impl->FindMeta(id); if(m) m->onTick=cb;
}
void StatusEffectSystem::SetOnExpire(uint32_t id,std::function<void(uint32_t)> cb){
    auto* m=m_impl->FindMeta(id); if(m) m->onExpire=cb;
}
void StatusEffectSystem::SetResistance(uint32_t entityId,uint32_t effectId,float chance){
    m_impl->entities[entityId].resistance[effectId]=std::max(0.f,std::min(1.f,chance));
}

} // namespace Runtime
