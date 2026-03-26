#include "Runtime/Combat/AbilitySystem/AbilitySystem.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct AbilitySystem::Impl {
    // entityId → ability instances
    std::unordered_map<uint32_t, std::vector<AbilityInstance>> abilities;
    // entityId → resources
    std::unordered_map<uint32_t, EntityResources> resources;
    // entityId → tags
    std::unordered_map<uint32_t, std::vector<std::string>> tags;

    std::function<void(uint32_t,const std::string&)> onActivated;
    std::function<void(uint32_t,const std::string&)> onEnded;
    std::function<void(uint32_t,const std::string&,const std::string&)> onFailed;

    AbilityInstance* Find(uint32_t entityId, const std::string& id) {
        auto it=abilities.find(entityId); if(it==abilities.end()) return nullptr;
        for(auto& a:it->second) if(a.desc.id==id) return &a;
        return nullptr;
    }
    bool HasTag(uint32_t entityId, const std::string& tag) const {
        auto it=tags.find(entityId); if(it==tags.end()) return false;
        return std::find(it->second.begin(),it->second.end(),tag)!=it->second.end();
    }
};

AbilitySystem::AbilitySystem() : m_impl(new Impl()) {}
AbilitySystem::~AbilitySystem() { delete m_impl; }
void AbilitySystem::Init()     {}
void AbilitySystem::Shutdown() {}

void AbilitySystem::RegisterEntity(uint32_t id, const EntityResources& r){ m_impl->resources[id]=r; }
void AbilitySystem::UnregisterEntity(uint32_t id){ m_impl->resources.erase(id); m_impl->abilities.erase(id); }
EntityResources AbilitySystem::GetResources(uint32_t id) const {
    auto it=m_impl->resources.find(id); return it!=m_impl->resources.end()?it->second:EntityResources{};
}
void AbilitySystem::SetResources(uint32_t id, const EntityResources& r){ m_impl->resources[id]=r; }

void AbilitySystem::Grant(uint32_t eid, const AbilityDesc& desc) {
    auto& list=m_impl->abilities[eid];
    for(auto& a:list) if(a.desc.id==desc.id) return;  // already granted
    AbilityInstance inst; inst.ownerId=eid; inst.desc=desc; inst.chargesRemaining=desc.charges;
    list.push_back(inst);
}
void AbilitySystem::Revoke(uint32_t eid, const std::string& id) {
    auto it=m_impl->abilities.find(eid); if(it==m_impl->abilities.end()) return;
    auto& v=it->second; v.erase(std::remove_if(v.begin(),v.end(),[&](auto& a){ return a.desc.id==id; }),v.end());
}
bool AbilitySystem::HasAbility(uint32_t eid, const std::string& id) const { return m_impl->Find(eid,id)!=nullptr; }
AbilityInstance AbilitySystem::GetAbility(uint32_t eid, const std::string& id) const {
    auto* p=const_cast<Impl*>(m_impl)->Find(eid,id); return p?*p:AbilityInstance{};
}
std::vector<AbilityInstance> AbilitySystem::GetAbilities(uint32_t eid) const {
    auto it=m_impl->abilities.find(eid); return it!=m_impl->abilities.end()?it->second:std::vector<AbilityInstance>{};
}

bool AbilitySystem::CanActivate(const std::string& id, uint32_t eid) const {
    auto* inst=const_cast<Impl*>(m_impl)->Find(eid,id);
    if(!inst||inst->state!=AbilityState::Ready||inst->chargesRemaining==0) return false;
    for(auto& tag: inst->desc.blockedByTags) if(m_impl->HasTag(eid,tag)) return false;
    auto rit=m_impl->resources.find(eid);
    if(rit!=m_impl->resources.end()) {
        if(rit->second.mana<inst->desc.manaCost) return false;
        if(rit->second.stamina<inst->desc.staminaCost) return false;
    }
    return true;
}

bool AbilitySystem::Activate(const std::string& id, uint32_t eid, uint32_t targetId, const float* targetPos) {
    if(!CanActivate(id,eid)){ if(m_impl->onFailed) m_impl->onFailed(eid,id,"cannot activate"); return false; }
    auto* inst=m_impl->Find(eid,id);
    // Deduct resources
    auto rit=m_impl->resources.find(eid);
    if(rit!=m_impl->resources.end()) {
        rit->second.mana=std::max(0.f,rit->second.mana-inst->desc.manaCost);
        rit->second.stamina=std::max(0.f,rit->second.stamina-inst->desc.staminaCost);
    }
    inst->chargesRemaining--;
    inst->state=inst->desc.duration>0.f?AbilityState::Active:AbilityState::Cooldown;
    inst->activeTime=0.f; inst->cooldownRemaining=0.f;
    AbilityContext ctx; ctx.casterId=eid; ctx.targetId=targetId;
    if(targetPos) for(int i=0;i<3;++i) ctx.targetPos[i]=targetPos[i];
    if(inst->desc.onActivate) inst->desc.onActivate(ctx);
    if(m_impl->onActivated) m_impl->onActivated(eid,id);
    return true;
}

void AbilitySystem::Cancel(const std::string& id, uint32_t eid) {
    auto* inst=m_impl->Find(eid,id); if(!inst||inst->state!=AbilityState::Active) return;
    AbilityContext ctx; ctx.casterId=eid;
    if(inst->desc.onInterrupt) inst->desc.onInterrupt(ctx);
    inst->state=AbilityState::Cooldown;
}
void AbilitySystem::CancelAll(uint32_t eid) {
    auto it=m_impl->abilities.find(eid); if(it==m_impl->abilities.end()) return;
    for(auto& a:it->second) if(a.state==AbilityState::Active) Cancel(a.desc.id,eid);
}
AbilityState AbilitySystem::GetState(const std::string& id, uint32_t eid) const {
    auto* p=const_cast<Impl*>(m_impl)->Find(eid,id); return p?p->state:AbilityState::Ready;
}

void AbilitySystem::AddTag(uint32_t eid, const std::string& tag){ m_impl->tags[eid].push_back(tag); }
void AbilitySystem::RemoveTag(uint32_t eid, const std::string& tag){
    auto it=m_impl->tags.find(eid); if(it==m_impl->tags.end()) return;
    auto& v=it->second; v.erase(std::remove(v.begin(),v.end(),tag),v.end());
}
bool AbilitySystem::HasTag(uint32_t eid, const std::string& tag) const{ return m_impl->HasTag(eid,tag); }

void AbilitySystem::Tick(float dt) {
    for(auto& [eid,list]: m_impl->abilities) {
        for(auto& inst: list) {
            if(inst.state==AbilityState::Active) {
                inst.activeTime+=dt;
                if(inst.desc.duration>0.f && inst.activeTime>=inst.desc.duration) {
                    AbilityContext ctx; ctx.casterId=eid;
                    if(inst.desc.onEnd) inst.desc.onEnd(ctx);
                    if(m_impl->onEnded) m_impl->onEnded(eid,inst.desc.id);
                    inst.state=AbilityState::Cooldown; inst.cooldownRemaining=inst.desc.cooldown;
                }
            } else if(inst.state==AbilityState::Cooldown) {
                inst.cooldownRemaining=std::max(0.f,inst.cooldownRemaining-dt);
                if(inst.cooldownRemaining<=0.f) {
                    if(inst.chargesRemaining<inst.desc.charges) ++inst.chargesRemaining;
                    if(inst.chargesRemaining>=inst.desc.charges) inst.state=AbilityState::Ready;
                    else inst.cooldownRemaining=inst.desc.cooldown;
                }
            }
        }
        // Regen
        auto rit=m_impl->resources.find(eid);
        if(rit!=m_impl->resources.end()) {
            rit->second.mana=std::min(rit->second.maxMana,rit->second.mana+rit->second.manaRegen*dt);
            rit->second.stamina=std::min(rit->second.maxStamina,rit->second.stamina+rit->second.staminaRegen*dt);
        }
    }
}

void AbilitySystem::OnActivated(std::function<void(uint32_t,const std::string&)> cb){ m_impl->onActivated=std::move(cb); }
void AbilitySystem::OnEnded(std::function<void(uint32_t,const std::string&)> cb)    { m_impl->onEnded=std::move(cb); }
void AbilitySystem::OnFailed(std::function<void(uint32_t,const std::string&,const std::string&)> cb){ m_impl->onFailed=std::move(cb); }

} // namespace Runtime
