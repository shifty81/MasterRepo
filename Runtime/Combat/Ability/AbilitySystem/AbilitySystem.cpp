#include "Runtime/Combat/Ability/AbilitySystem/AbilitySystem.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct AbilityInstance {
    uint32_t abilityId;
    float    cooldownRemaining{0};
    uint32_t charges;
    bool     isCasting{false};
    float    castRemaining{0};
    float    cdMult{1.f};
};

struct EntityAbilityData {
    std::unordered_map<uint32_t,AbilityInstance> instances;
    std::unordered_map<uint32_t,float>           resources; // resType → amount
};

struct AbilityMeta {
    AbilityDef def;
    std::function<void(uint32_t)> onActivate, onComplete, onCancelled;
};

struct AbilitySystem::Impl {
    std::unordered_map<uint32_t,AbilityMeta>      defs;
    std::unordered_map<uint32_t,EntityAbilityData> entities;

    AbilityMeta* FindMeta(uint32_t id){ auto it=defs.find(id); return it!=defs.end()?&it->second:nullptr; }
    AbilityInstance* FindInst(uint32_t entity, uint32_t abilityId){
        auto eit=entities.find(entity); if(eit==entities.end()) return nullptr;
        auto it=eit->second.instances.find(abilityId);
        return it!=eit->second.instances.end()?&it->second:nullptr;
    }
};

AbilitySystem::AbilitySystem(): m_impl(new Impl){}
AbilitySystem::~AbilitySystem(){ Shutdown(); delete m_impl; }
void AbilitySystem::Init(){}
void AbilitySystem::Shutdown(){ m_impl->defs.clear(); m_impl->entities.clear(); }
void AbilitySystem::Reset(){ Shutdown(); }

bool AbilitySystem::RegisterAbility(const AbilityDef& def){
    if(m_impl->defs.count(def.id)) return false;
    AbilityMeta m; m.def=def; m_impl->defs[def.id]=m; return true;
}

void AbilitySystem::GrantAbility(uint32_t entity, uint32_t abilityId){
    auto* meta=m_impl->FindMeta(abilityId); if(!meta) return;
    auto& inst=m_impl->entities[entity].instances[abilityId];
    inst.abilityId=abilityId;
    inst.charges=meta->def.maxCharges;
}
void AbilitySystem::RevokeAbility(uint32_t entity, uint32_t abilityId){
    auto it=m_impl->entities.find(entity); if(it==m_impl->entities.end()) return;
    it->second.instances.erase(abilityId);
}
bool AbilitySystem::HasAbility(uint32_t entity, uint32_t abilityId) const {
    auto it=m_impl->entities.find(entity); if(it==m_impl->entities.end()) return false;
    return it->second.instances.count(abilityId)>0;
}

bool AbilitySystem::ActivateAbility(uint32_t entity, uint32_t abilityId){
    auto* inst=m_impl->FindInst(entity,abilityId); if(!inst) return false;
    auto* meta=m_impl->FindMeta(abilityId); if(!meta) return false;
    if(inst->cooldownRemaining>0||inst->charges==0) return false;
    // Cost check
    if(meta->def.cost>0){
        auto& res=m_impl->entities[entity].resources[meta->def.resourceType];
        if(res<meta->def.cost) return false;
        res-=meta->def.cost;
    }
    inst->charges--;
    if(meta->def.castTime>0){
        inst->isCasting=true; inst->castRemaining=meta->def.castTime;
    } else {
        inst->cooldownRemaining=meta->def.cooldown*inst->cdMult;
        if(meta->onActivate) meta->onActivate(entity);
        if(meta->onComplete) meta->onComplete(entity);
    }
    return true;
}
void AbilitySystem::CancelAbility(uint32_t entity, uint32_t abilityId){
    auto* inst=m_impl->FindInst(entity,abilityId); if(!inst||!inst->isCasting) return;
    inst->isCasting=false; inst->castRemaining=0;
    auto* meta=m_impl->FindMeta(abilityId);
    if(meta&&meta->onCancelled) meta->onCancelled(entity);
}

void AbilitySystem::Tick(float dt){
    for(auto& [entity,data]:m_impl->entities){
        for(auto& [abilityId,inst]:data.instances){
            auto* meta=m_impl->FindMeta(abilityId); if(!meta) continue;
            if(inst.isCasting){
                inst.castRemaining-=dt;
                if(inst.castRemaining<=0){
                    inst.isCasting=false;
                    inst.cooldownRemaining=meta->def.cooldown*inst.cdMult;
                    if(meta->onActivate) meta->onActivate(entity);
                    if(meta->onComplete) meta->onComplete(entity);
                }
            } else if(inst.cooldownRemaining>0){
                inst.cooldownRemaining-=dt;
                if(inst.cooldownRemaining<=0){
                    inst.cooldownRemaining=0;
                    inst.charges=std::min(inst.charges+1,meta->def.maxCharges);
                }
            }
        }
    }
}

bool     AbilitySystem::IsOnCooldown(uint32_t e, uint32_t id) const { auto* i=m_impl->FindInst(e,id); return i&&i->cooldownRemaining>0; }
float    AbilitySystem::GetCooldownRemaining(uint32_t e, uint32_t id) const { auto* i=m_impl->FindInst(e,id); return i?i->cooldownRemaining:0; }
uint32_t AbilitySystem::GetChargesRemaining(uint32_t e, uint32_t id) const { auto* i=m_impl->FindInst(e,id); return i?i->charges:0; }
uint32_t AbilitySystem::GetGrantedAbilities(uint32_t entity, std::vector<uint32_t>& out) const {
    out.clear(); auto it=m_impl->entities.find(entity); if(it==m_impl->entities.end()) return 0;
    for(auto& [id,_]:it->second.instances) out.push_back(id);
    return (uint32_t)out.size();
}
void AbilitySystem::AddCooldownModifier(uint32_t e, uint32_t id, float mult){
    auto* i=m_impl->FindInst(e,id); if(i) i->cdMult=mult;
}
void AbilitySystem::SetResource(uint32_t e, uint32_t resType, float amt){
    m_impl->entities[e].resources[resType]=std::max(0.f,amt);
}
float AbilitySystem::GetResource(uint32_t e, uint32_t resType) const {
    auto it=m_impl->entities.find(e); if(it==m_impl->entities.end()) return 0;
    auto rit=it->second.resources.find(resType); return rit!=it->second.resources.end()?rit->second:0;
}
void AbilitySystem::SetOnActivate(uint32_t id, std::function<void(uint32_t)> cb){ auto* m=m_impl->FindMeta(id); if(m) m->onActivate=cb; }
void AbilitySystem::SetOnComplete(uint32_t id, std::function<void(uint32_t)> cb){ auto* m=m_impl->FindMeta(id); if(m) m->onComplete=cb; }
void AbilitySystem::SetOnCancelled(uint32_t id, std::function<void(uint32_t)> cb){ auto* m=m_impl->FindMeta(id); if(m) m->onCancelled=cb; }

} // namespace Runtime
