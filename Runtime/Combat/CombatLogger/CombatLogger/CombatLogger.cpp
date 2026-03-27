#include "Runtime/Combat/CombatLogger/CombatLogger/CombatLogger.h"
#include <algorithm>
#include <sstream>
#include <vector>

namespace Runtime {

struct CombatLogger::Impl {
    std::vector<CombatEvent> events;
    uint32_t maxEvents{10000};
    float    timestamp{0};
    std::function<void(const CombatEvent&)> onEvent;

    void Push(CombatEvent ev){
        ev.timestamp=timestamp;
        if(events.size()>=maxEvents) events.erase(events.begin());
        events.push_back(ev);
        if(onEvent) onEvent(events.back());
    }
};

CombatLogger::CombatLogger(): m_impl(new Impl){}
CombatLogger::~CombatLogger(){ Shutdown(); delete m_impl; }
void CombatLogger::Init(){}
void CombatLogger::Shutdown(){}
void CombatLogger::Clear(){ m_impl->events.clear(); }

void CombatLogger::LogDamage(uint32_t att,uint32_t tgt,float amt,uint32_t type,bool crit){
    CombatEvent ev; ev.type=CombatEventType::Damage;
    ev.attackerId=att; ev.targetId=tgt; ev.amount=amt; ev.abilityId=type; ev.isCrit=crit;
    m_impl->Push(ev);
}
void CombatLogger::LogHeal(uint32_t caster,uint32_t tgt,float amt,bool crit){
    CombatEvent ev; ev.type=CombatEventType::Heal;
    ev.attackerId=caster; ev.targetId=tgt; ev.amount=amt; ev.isCrit=crit;
    m_impl->Push(ev);
}
void CombatLogger::LogKill(uint32_t killer,uint32_t tgt){
    CombatEvent ev; ev.type=CombatEventType::Kill;
    ev.attackerId=killer; ev.targetId=tgt;
    m_impl->Push(ev);
}
void CombatLogger::LogAbility(uint32_t caster,uint32_t abilityId,uint32_t tgt){
    CombatEvent ev; ev.type=CombatEventType::AbilityUse;
    ev.attackerId=caster; ev.abilityId=abilityId; ev.targetId=tgt;
    m_impl->Push(ev);
}
void CombatLogger::LogStatus(uint32_t caster,uint32_t tgt,uint32_t effectId,bool applied){
    CombatEvent ev; ev.type=applied?CombatEventType::StatusApply:CombatEventType::StatusExpire;
    ev.attackerId=caster; ev.targetId=tgt; ev.effectId=effectId;
    m_impl->Push(ev);
}

uint32_t  CombatLogger::GetEventCount() const { return (uint32_t)m_impl->events.size(); }
CombatEvent CombatLogger::GetEvent(uint32_t i) const { return i<m_impl->events.size()?m_impl->events[i]:CombatEvent{}; }

std::vector<CombatEvent> CombatLogger::Filter(uint32_t entityId) const {
    std::vector<CombatEvent> out;
    for(auto& e:m_impl->events) if(e.attackerId==entityId||e.targetId==entityId) out.push_back(e);
    return out;
}
std::vector<CombatEvent> CombatLogger::FilterType(CombatEventType t) const {
    std::vector<CombatEvent> out;
    for(auto& e:m_impl->events) if(e.type==t) out.push_back(e);
    return out;
}

float CombatLogger::GetTotalDamageDone(uint32_t id) const {
    float sum=0;
    for(auto& e:m_impl->events) if(e.type==CombatEventType::Damage&&e.attackerId==id) sum+=e.amount;
    return sum;
}
float CombatLogger::GetTotalDamageTaken(uint32_t id) const {
    float sum=0;
    for(auto& e:m_impl->events) if(e.type==CombatEventType::Damage&&e.targetId==id) sum+=e.amount;
    return sum;
}
float CombatLogger::GetTotalHealDone(uint32_t id) const {
    float sum=0;
    for(auto& e:m_impl->events) if(e.type==CombatEventType::Heal&&e.attackerId==id) sum+=e.amount;
    return sum;
}
uint32_t CombatLogger::GetKillCount(uint32_t id) const {
    uint32_t n=0;
    for(auto& e:m_impl->events) if(e.type==CombatEventType::Kill&&e.attackerId==id) n++;
    return n;
}

std::string CombatLogger::ExportCSV() const {
    std::ostringstream ss;
    ss<<"type,attackerId,targetId,abilityId,effectId,amount,isCrit,timestamp\n";
    for(auto& e:m_impl->events){
        ss<<(int)e.type<<","<<e.attackerId<<","<<e.targetId<<","
          <<e.abilityId<<","<<e.effectId<<","<<e.amount<<","
          <<(int)e.isCrit<<","<<e.timestamp<<"\n";
    }
    return ss.str();
}

void CombatLogger::SetMaxEvents(uint32_t n){ m_impl->maxEvents=std::max(1u,n); }
void CombatLogger::SetOnEvent(std::function<void(const CombatEvent&)> cb){ m_impl->onEvent=cb; }
void CombatLogger::SetTimestamp(float t){ m_impl->timestamp=t; }

} // namespace Runtime
