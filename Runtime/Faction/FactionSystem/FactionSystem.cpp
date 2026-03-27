#include "Runtime/Faction/FactionSystem/FactionSystem.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Runtime {

using PairKey = std::pair<std::string,std::string>;
struct PairHash {
    size_t operator()(const PairKey& p) const {
        return std::hash<std::string>{}(p.first) ^ (std::hash<std::string>{}(p.second)<<32);
    }
};

struct FactionSystem::Impl {
    std::vector<FactionDef> factions;
    std::unordered_map<PairKey, float, PairHash>    reputation; // normalized per pair
    std::unordered_map<uint32_t, std::string>       entityFaction;
    std::function<void(const std::string&,const std::string&,Standing,Standing)> onChanged;
    float threshUnfriendly{-200.f};
    float threshFriendly  { 200.f};
    float threshAllied    { 600.f};

    FactionDef* Find(const std::string& id){
        for(auto& f:factions) if(f.id==id) return &f; return nullptr;
    }
    const FactionDef* Find(const std::string& id) const {
        for(auto& f:factions) if(f.id==id) return &f; return nullptr;
    }

    PairKey Key(const std::string& a, const std::string& b) const {
        return (a<b)?PairKey{a,b}:PairKey{b,a};
    }

    Standing ReputationToStanding(float rep) const {
        if(rep <= threshUnfriendly*2) return Standing::Hostile;
        if(rep <= threshUnfriendly)   return Standing::Unfriendly;
        if(rep >= threshAllied)       return Standing::Allied;
        if(rep >= threshFriendly)     return Standing::Friendly;
        return Standing::Neutral;
    }
};

FactionSystem::FactionSystem()  : m_impl(new Impl){}
FactionSystem::~FactionSystem() { Shutdown(); delete m_impl; }

void FactionSystem::Init()     {}
void FactionSystem::Shutdown() { m_impl->factions.clear(); m_impl->reputation.clear(); }

void FactionSystem::RegisterFaction(const FactionDef& def){
    if(!m_impl->Find(def.id)) m_impl->factions.push_back(def);
}
void FactionSystem::UnregisterFaction(const std::string& id){
    auto& v=m_impl->factions;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& f){return f.id==id;}),v.end());
}
bool FactionSystem::HasFaction(const std::string& id) const { return m_impl->Find(id)!=nullptr; }
const FactionDef* FactionSystem::GetFaction(const std::string& id) const { return m_impl->Find(id); }
std::vector<FactionDef> FactionSystem::GetAll() const { return m_impl->factions; }

void FactionSystem::SetStanding(const std::string& a, const std::string& b, Standing s){
    float rep=0.f;
    switch(s){
        case Standing::Hostile:    rep= m_impl->threshUnfriendly*3; break;
        case Standing::Unfriendly: rep=(m_impl->threshUnfriendly+m_impl->threshUnfriendly*2)/2; break;
        case Standing::Neutral:    rep=0.f; break;
        case Standing::Friendly:   rep=(m_impl->threshFriendly+m_impl->threshAllied)/2; break;
        case Standing::Allied:     rep=m_impl->threshAllied+100.f; break;
    }
    m_impl->reputation[m_impl->Key(a,b)]=rep;
}

Standing FactionSystem::GetStanding(const std::string& a, const std::string& b) const {
    auto it=m_impl->reputation.find(m_impl->Key(a,b));
    return m_impl->ReputationToStanding(it!=m_impl->reputation.end()?it->second:0.f);
}

void FactionSystem::AdjustReputation(const std::string& a, const std::string& b, float delta){
    auto key=m_impl->Key(a,b);
    Standing old=GetStanding(a,b);
    float& rep=m_impl->reputation[key];
    rep=std::max(-1000.f,std::min(1000.f, rep+delta));
    Standing now=GetStanding(a,b);
    if(old!=now && m_impl->onChanged) m_impl->onChanged(a,b,old,now);
}

float FactionSystem::GetReputation(const std::string& a, const std::string& b) const {
    auto it=m_impl->reputation.find(m_impl->Key(a,b));
    return it!=m_impl->reputation.end()?it->second:0.f;
}

bool FactionSystem::IsHostile   (const std::string& a, const std::string& b) const { return GetStanding(a,b)==Standing::Hostile; }
bool FactionSystem::IsUnfriendly(const std::string& a, const std::string& b) const { return GetStanding(a,b)==Standing::Unfriendly; }
bool FactionSystem::IsFriendly  (const std::string& a, const std::string& b) const { return GetStanding(a,b)==Standing::Friendly; }
bool FactionSystem::IsAllied    (const std::string& a, const std::string& b) const { return GetStanding(a,b)==Standing::Allied; }

void FactionSystem::DeclareWar(const std::string& a, const std::string& b){
    SetStanding(a,b,Standing::Hostile);
    if(m_impl->onChanged) m_impl->onChanged(a,b,Standing::Neutral,Standing::Hostile);
}
void FactionSystem::DeclarePeace(const std::string& a, const std::string& b){
    SetStanding(a,b,Standing::Neutral);
}

void FactionSystem::AssignFaction(uint32_t entityId, const std::string& fid){
    m_impl->entityFaction[entityId]=fid;
}
void FactionSystem::RemoveFaction(uint32_t entityId){
    m_impl->entityFaction.erase(entityId);
}
std::string FactionSystem::GetFactionOf(uint32_t entityId) const {
    auto it=m_impl->entityFaction.find(entityId);
    return it!=m_impl->entityFaction.end()?it->second:"";
}
bool FactionSystem::EntitiesHostile(uint32_t e1, uint32_t e2) const {
    return IsHostile(GetFactionOf(e1), GetFactionOf(e2));
}

void FactionSystem::SetThresholds(float u, float f, float a){
    m_impl->threshUnfriendly=u; m_impl->threshFriendly=f; m_impl->threshAllied=a;
}

void FactionSystem::SetOnStandingChanged(
    std::function<void(const std::string&,const std::string&,Standing,Standing)> cb){
    m_impl->onChanged=cb;
}

} // namespace Runtime
