#include "Runtime/Combat/StatusEffects/StatusEffectSystem.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct StatusEffectSystem::Impl {
    std::unordered_map<std::string, StatusEffectDef> defs;
    // entityId → active effects
    std::unordered_map<uint32_t, std::vector<ActiveEffect>> active;
    // entityId → immunity tags
    std::unordered_map<uint32_t, std::vector<std::string>> immunity;

    std::function<void(uint32_t,const std::string&)> onApplied;
    std::function<void(uint32_t,const std::string&)> onExpired;
};

StatusEffectSystem::StatusEffectSystem() : m_impl(new Impl()) {}
StatusEffectSystem::~StatusEffectSystem() { delete m_impl; }
void StatusEffectSystem::Init()     {}
void StatusEffectSystem::Shutdown() {}

void StatusEffectSystem::RegisterDef(const StatusEffectDef& def){ m_impl->defs[def.id]=def; }
void StatusEffectSystem::UnregisterDef(const std::string& id){ m_impl->defs.erase(id); }
bool StatusEffectSystem::HasDef(const std::string& id) const{ return m_impl->defs.count(id)>0; }
StatusEffectDef StatusEffectSystem::GetDef(const std::string& id) const {
    auto it=m_impl->defs.find(id); return it!=m_impl->defs.end()?it->second:StatusEffectDef{};
}

bool StatusEffectSystem::Apply(uint32_t eid, const std::string& effectId, uint32_t srcId) {
    auto dit=m_impl->defs.find(effectId); if(dit==m_impl->defs.end()) return false;
    auto& def=dit->second;
    // Immunity check
    if(IsImmune(eid,effectId)) return false;
    auto& list=m_impl->active[eid];
    // Find existing
    for(auto& ae: list) {
        if(ae.defId==effectId) {
            if(def.stackMode==StackMode::Refresh) {
                ae.timeRemaining=def.duration;
            } else if(def.stackMode==StackMode::Additive) {
                ae.stacks=std::min(ae.stacks+1,def.maxStacks);
            } else { // Max
                ae.timeRemaining=std::max(ae.timeRemaining,def.duration);
            }
            return true;
        }
    }
    // New effect
    ActiveEffect ae; ae.defId=effectId; ae.sourceId=srcId;
    ae.timeRemaining=def.duration; ae.stacks=1;
    list.push_back(ae);
    if(def.onApply) def.onApply(eid);
    if(m_impl->onApplied) m_impl->onApplied(eid,effectId);
    return true;
}

bool StatusEffectSystem::Remove(uint32_t eid, const std::string& effectId) {
    auto it=m_impl->active.find(eid); if(it==m_impl->active.end()) return false;
    auto& v=it->second;
    auto jt=std::find_if(v.begin(),v.end(),[&](auto& ae){ return ae.defId==effectId; });
    if(jt==v.end()) return false;
    auto dit=m_impl->defs.find(effectId);
    if(dit!=m_impl->defs.end()&&dit->second.onRemove) dit->second.onRemove(eid);
    v.erase(jt);
    return true;
}
void StatusEffectSystem::RemoveAll(uint32_t eid){ m_impl->active.erase(eid); }

bool StatusEffectSystem::HasEffect(uint32_t eid, const std::string& id) const {
    auto it=m_impl->active.find(eid); if(it==m_impl->active.end()) return false;
    return std::any_of(it->second.begin(),it->second.end(),[&](auto& ae){ return ae.defId==id; });
}
uint32_t StatusEffectSystem::GetStacks(uint32_t eid, const std::string& id) const {
    auto it=m_impl->active.find(eid); if(it==m_impl->active.end()) return 0;
    for(auto& ae:it->second) if(ae.defId==id) return ae.stacks;
    return 0;
}
float StatusEffectSystem::GetTimeRemaining(uint32_t eid, const std::string& id) const {
    auto it=m_impl->active.find(eid); if(it==m_impl->active.end()) return 0.f;
    for(auto& ae:it->second) if(ae.defId==id) return ae.timeRemaining;
    return 0.f;
}
std::vector<ActiveEffect> StatusEffectSystem::GetEffects(uint32_t eid) const {
    auto it=m_impl->active.find(eid); return it!=m_impl->active.end()?it->second:std::vector<ActiveEffect>{};
}

void StatusEffectSystem::AddImmunity(uint32_t eid, const std::string& tag){ m_impl->immunity[eid].push_back(tag); }
void StatusEffectSystem::RemoveImmunity(uint32_t eid, const std::string& tag){
    auto it=m_impl->immunity.find(eid); if(it==m_impl->immunity.end()) return;
    auto& v=it->second; v.erase(std::remove(v.begin(),v.end(),tag),v.end());
}
bool StatusEffectSystem::IsImmune(uint32_t eid, const std::string& effectId) const {
    auto dit=m_impl->defs.find(effectId); if(dit==m_impl->defs.end()) return false;
    auto iit=m_impl->immunity.find(eid); if(iit==m_impl->immunity.end()) return false;
    for(auto& it: dit->second.immunityTags)
        if(std::find(iit->second.begin(),iit->second.end(),it)!=iit->second.end()) return true;
    return false;
}

void StatusEffectSystem::Tick(float dt) {
    for(auto& [eid,list]: m_impl->active) {
        for(auto& ae: list) {
            if(ae.defId.empty()) continue;
            auto dit=m_impl->defs.find(ae.defId); if(dit==m_impl->defs.end()) continue;
            auto& def=dit->second;
            // Tick
            if(def.tickRate>0.f) {
                ae.tickAccum+=dt;
                while(ae.tickAccum>=def.tickRate) {
                    ae.tickAccum-=def.tickRate;
                    if(def.onTick) def.onTick(eid,def.tickRate);
                }
            }
            // Duration
            if(def.duration>0.f) {
                ae.timeRemaining=std::max(0.f,ae.timeRemaining-dt);
            }
        }
        // Expire
        list.erase(std::remove_if(list.begin(),list.end(),[&](ActiveEffect& ae){
            if(ae.defId.empty()) return true;
            auto dit=m_impl->defs.find(ae.defId);
            if(dit==m_impl->defs.end()) return true;
            if(dit->second.duration>0.f && ae.timeRemaining<=0.f) {
                if(dit->second.onExpire) dit->second.onExpire(eid);
                if(m_impl->onExpired) m_impl->onExpired(eid,ae.defId);
                return true;
            }
            return false;
        }),list.end());
    }
}

void StatusEffectSystem::OnEffectApplied(std::function<void(uint32_t,const std::string&)> cb){ m_impl->onApplied=std::move(cb); }
void StatusEffectSystem::OnEffectExpired(std::function<void(uint32_t,const std::string&)> cb){ m_impl->onExpired=std::move(cb); }

} // namespace Runtime
