#include "Runtime/Combat/DamagePipeline/DamagePipeline.h"
#include <algorithm>
#include <cstdlib>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct EntityHP {
    float hp{100.f};
    float maxHp{100.f};
};

struct DamagePipeline::Impl {
    std::vector<DamageModifier>                         modifiers;
    std::unordered_map<uint32_t, std::unordered_map<uint8_t,float>> resistances;
    std::unordered_map<uint32_t, EntityHP>              entities;

    std::function<void(const DamageResult&)>        onDamage;
    std::function<void(uint32_t,uint32_t)>          onDied;

    float ApplyModifiers(float base, DamageType type, uint32_t sourceId) const {
        float flat=0.f, pct=0.f;
        for(auto& m: modifiers) {
            bool typeMatch = m.affectsType==DamageType::COUNT || m.affectsType==type;
            bool entityMatch = m.isGlobal || m.entityId==sourceId;
            if(typeMatch && entityMatch) { flat+=m.flatBonus; pct+=m.percentBonus; }
        }
        return (base*(1.f+pct))+flat;
    }

    DamageResult Process(const DamageRequest& req, bool apply) {
        DamageResult r;
        r.targetId=req.targetId; r.type=req.type; r.rawAmount=req.amount;
        float amount=ApplyModifiers(req.amount, req.type, req.sourceId);
        // Crit
        bool crit=req.isCritical;
        if(!crit && (std::rand()%100<5)) crit=true; // 5% base crit
        if(crit) { amount*=req.critMultiplier; r.wasCritical=true; }
        // Resistance
        float res=0.f;
        auto rit=resistances.find(req.targetId);
        if(rit!=resistances.end()) {
            auto tit=rit->second.find((uint8_t)req.type);
            if(tit!=rit->second.end()) res=tit->second;
        }
        if(req.type==DamageType::True) res=0.f;
        float absorbed=amount*std::min(1.f,std::max(0.f,res));
        float final=amount-absorbed;
        r.finalAmount=final; r.absorbed=absorbed;
        if(apply) {
            auto eit=entities.find(req.targetId);
            if(eit!=entities.end()) {
                eit->second.hp=std::max(0.f,eit->second.hp-final);
                if(eit->second.hp<=0.f) { r.killed=true; if(onDied) onDied(req.targetId,req.sourceId); }
            }
            if(onDamage) onDamage(r);
        }
        return r;
    }
};

DamagePipeline::DamagePipeline() : m_impl(new Impl()) {}
DamagePipeline::~DamagePipeline() { delete m_impl; }
void DamagePipeline::Init()     {}
void DamagePipeline::Shutdown() {}

void DamagePipeline::AddModifier(const DamageModifier& m){ m_impl->modifiers.push_back(m); }
void DamagePipeline::RemoveModifier(const std::string& id){
    auto& v=m_impl->modifiers;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& m){ return m.id==id; }),v.end());
}
void DamagePipeline::ClearModifiers(){ m_impl->modifiers.clear(); }

void DamagePipeline::SetResistance(uint32_t eid, DamageType t, float r){
    m_impl->resistances[eid][(uint8_t)t]=std::min(1.f,std::max(0.f,r));
}
float DamagePipeline::GetResistance(uint32_t eid, DamageType t) const {
    auto it=m_impl->resistances.find(eid); if(it==m_impl->resistances.end()) return 0.f;
    auto jt=it->second.find((uint8_t)t); return jt!=it->second.end()?jt->second:0.f;
}
void DamagePipeline::ClearResistances(uint32_t eid){ m_impl->resistances.erase(eid); }

DamageResult DamagePipeline::Apply(const DamageRequest& req){ return m_impl->Process(req,true); }
DamageResult DamagePipeline::Preview(const DamageRequest& req) const { return const_cast<Impl*>(m_impl)->Process(req,false); }

void DamagePipeline::RegisterEntity(uint32_t id, float maxHp){ m_impl->entities[id]={maxHp,maxHp}; }
void DamagePipeline::UnregisterEntity(uint32_t id){ m_impl->entities.erase(id); }
bool DamagePipeline::IsAlive(uint32_t id) const {
    auto it=m_impl->entities.find(id); return it!=m_impl->entities.end()&&it->second.hp>0.f;
}
float DamagePipeline::GetHealth(uint32_t id) const {
    auto it=m_impl->entities.find(id); return it!=m_impl->entities.end()?it->second.hp:0.f;
}
float DamagePipeline::GetMaxHealth(uint32_t id) const {
    auto it=m_impl->entities.find(id); return it!=m_impl->entities.end()?it->second.maxHp:0.f;
}
void DamagePipeline::SetHealth(uint32_t id, float hp){
    auto it=m_impl->entities.find(id); if(it!=m_impl->entities.end()) it->second.hp=std::min(hp,it->second.maxHp);
}
void DamagePipeline::Heal(uint32_t id, float amount){
    auto it=m_impl->entities.find(id);
    if(it!=m_impl->entities.end()) it->second.hp=std::min(it->second.hp+amount,it->second.maxHp);
}

void DamagePipeline::OnDamageApplied(std::function<void(const DamageResult&)> cb){ m_impl->onDamage=std::move(cb); }
void DamagePipeline::OnEntityDied(std::function<void(uint32_t,uint32_t)> cb){ m_impl->onDied=std::move(cb); }

} // namespace Runtime
