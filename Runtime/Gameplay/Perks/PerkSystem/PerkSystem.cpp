#include "Runtime/Gameplay/Perks/PerkSystem/PerkSystem.h"
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct PerkSystem::Impl {
    std::unordered_map<std::string,PerkDef> defs;
    std::unordered_map<uint32_t, std::unordered_map<std::string,uint32_t>> playerPerks; // playerId→(perkId→stacks)
    std::function<void(uint32_t,const std::string&)> onUnlock;
};

PerkSystem::PerkSystem()  : m_impl(new Impl){}
PerkSystem::~PerkSystem() { Shutdown(); delete m_impl; }
void PerkSystem::Init()     {}
void PerkSystem::Shutdown() { m_impl->defs.clear(); m_impl->playerPerks.clear(); }

void PerkSystem::RegisterPerk(const PerkDef& d){ m_impl->defs[d.id]=d; }
void PerkSystem::UnregisterPerk(const std::string& id){ m_impl->defs.erase(id); }
const PerkDef* PerkSystem::GetPerkDef(const std::string& id) const {
    auto it=m_impl->defs.find(id); return it!=m_impl->defs.end()?&it->second:nullptr;
}
std::vector<std::string> PerkSystem::GetAllPerkIds() const {
    std::vector<std::string> out; for(auto& [k,v]:m_impl->defs) out.push_back(k); return out;
}

bool PerkSystem::CanUnlock(uint32_t pid, const std::string& perkId) const {
    auto* def=GetPerkDef(perkId); if(!def) return false;
    auto& pp=m_impl->playerPerks[pid];
    // Check max stacks
    auto it=pp.find(perkId); if(it!=pp.end()&&it->second>=def->maxStacks) return false;
    // Check prerequisites
    for(auto& req:def->prerequisiteIds){ auto r=pp.find(req); if(r==pp.end()||r->second==0) return false; }
    return true;
}

bool PerkSystem::UnlockPerk(uint32_t pid, const std::string& perkId){
    if(!CanUnlock(pid,perkId)) return false;
    m_impl->playerPerks[pid][perkId]++;
    if(m_impl->onUnlock) m_impl->onUnlock(pid,perkId);
    return true;
}
void PerkSystem::LockPerk(uint32_t pid, const std::string& perkId, bool all){
    auto& pp=m_impl->playerPerks[pid]; auto it=pp.find(perkId); if(it==pp.end()) return;
    if(all||it->second<=1) pp.erase(it); else it->second--;
}
bool PerkSystem::IsUnlocked(uint32_t pid, const std::string& perkId) const {
    auto& pp=m_impl->playerPerks[pid]; auto it=pp.find(perkId); return it!=pp.end()&&it->second>0;
}
uint32_t PerkSystem::GetStackCount(uint32_t pid, const std::string& perkId) const {
    auto& pp=m_impl->playerPerks[pid]; auto it=pp.find(perkId); return it!=pp.end()?it->second:0;
}

std::vector<PerkState> PerkSystem::GetPlayerPerks(uint32_t pid) const {
    std::vector<PerkState> out;
    for(auto& [id,stacks]:m_impl->playerPerks[pid]) out.push_back({id,stacks});
    return out;
}
std::vector<std::string> PerkSystem::GetAvailablePerks(uint32_t pid) const {
    std::vector<std::string> out;
    for(auto& [id,_]:m_impl->defs) if(!IsUnlocked(pid,id)&&CanUnlock(pid,id)) out.push_back(id);
    return out;
}

void PerkSystem::SetOnUnlock(std::function<void(uint32_t,const std::string&)> cb){ m_impl->onUnlock=cb; }

std::string PerkSystem::SaveState(uint32_t pid) const {
    std::ostringstream ss; ss<<"{";
    bool first=true;
    for(auto& [id,stacks]:m_impl->playerPerks[pid]){
        if(!first) ss<<","; first=false;
        ss<<"\""<<id<<"\":"<<stacks;
    }
    ss<<"}"; return ss.str();
}
void PerkSystem::LoadState(uint32_t pid, const std::string& json){
    // Simple stub parser (key:value pairs)
    m_impl->playerPerks[pid].clear();
    std::string s=json;
    size_t pos=0;
    while((pos=s.find('"',pos))!=std::string::npos){
        size_t end=s.find('"',pos+1); if(end==std::string::npos) break;
        std::string key=s.substr(pos+1,end-pos-1); pos=end+1;
        size_t col=s.find(':',pos); if(col==std::string::npos) break;
        size_t numEnd=s.find_first_of(",}",col+1);
        uint32_t val=(uint32_t)std::stoi(s.substr(col+1,numEnd-col-1));
        m_impl->playerPerks[pid][key]=val; pos=numEnd;
    }
}
void PerkSystem::ResetPlayer(uint32_t pid){ m_impl->playerPerks[pid].clear(); }

} // namespace Runtime
