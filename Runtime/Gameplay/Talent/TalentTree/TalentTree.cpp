#include "Runtime/Gameplay/Talent/TalentTree/TalentTree.h"
#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace Runtime {

struct TalentTree::Impl {
    std::unordered_map<std::string,TalentNode> nodes;
    std::unordered_map<uint32_t,uint32_t>      totalPoints;
    std::unordered_map<uint32_t,std::unordered_map<std::string,uint32_t>> ranks;
    std::function<void(uint32_t,const std::string&,uint32_t)> onActivate;
};

TalentTree::TalentTree() : m_impl(new Impl){}
TalentTree::~TalentTree(){ Shutdown(); delete m_impl; }
void TalentTree::Init()     {}
void TalentTree::Shutdown() { m_impl->nodes.clear(); m_impl->totalPoints.clear(); m_impl->ranks.clear(); }

void TalentTree::RegisterNode(const TalentNode& d){ m_impl->nodes[d.id]=d; }
void TalentTree::UnregisterNode(const std::string& id){ m_impl->nodes.erase(id); }
const TalentNode* TalentTree::GetNode(const std::string& id) const { auto it=m_impl->nodes.find(id); return it!=m_impl->nodes.end()?&it->second:nullptr; }
std::vector<std::string> TalentTree::GetAllNodeIds() const { std::vector<std::string> v; for(auto& [k,_]:m_impl->nodes) v.push_back(k); return v; }

void TalentTree::AddPoints(uint32_t pid, uint32_t n){ m_impl->totalPoints[pid]+=n; }

bool TalentTree::CanSpend(uint32_t pid, const std::string& id) const {
    auto* def=GetNode(id); if(!def) return false;
    auto& r=m_impl->ranks[pid];
    auto it=r.find(id); uint32_t rank=it!=r.end()?it->second:0;
    if(rank>=def->maxRank) return false;
    if(GetRemainingPoints(pid)<def->pointCost) return false;
    for(auto& pre:def->prerequisiteIds){ auto p=r.find(pre); if(p==r.end()||p->second==0) return false; }
    return true;
}
bool TalentTree::Spend(uint32_t pid, const std::string& id){
    if(!CanSpend(pid,id)) return false;
    auto* def=GetNode(id);
    m_impl->ranks[pid][id]++;
    m_impl->totalPoints[pid]-=def->pointCost;
    uint32_t rank=m_impl->ranks[pid][id];
    if(m_impl->onActivate) m_impl->onActivate(pid,id,rank);
    return true;
}
bool TalentTree::IsActive(uint32_t pid, const std::string& id) const { auto& r=m_impl->ranks[pid]; auto it=r.find(id); return it!=r.end()&&it->second>0; }
uint32_t TalentTree::GetRank(uint32_t pid, const std::string& id) const { auto& r=m_impl->ranks[pid]; auto it=r.find(id); return it!=r.end()?it->second:0; }
uint32_t TalentTree::GetSpentPoints(uint32_t pid) const {
    uint32_t s=0; for(auto& [id,rk]:m_impl->ranks[pid]){ auto* d=GetNode(id); if(d) s+=rk*d->pointCost; } return s;
}
uint32_t TalentTree::GetRemainingPoints(uint32_t pid) const { return m_impl->totalPoints.count(pid)?m_impl->totalPoints.at(pid):0; }
void TalentTree::Respec(uint32_t pid){ uint32_t sp=GetSpentPoints(pid); m_impl->ranks[pid].clear(); m_impl->totalPoints[pid]+=sp; }
std::vector<std::string> TalentTree::GetAvailableNodes(uint32_t pid) const {
    std::vector<std::string> v; for(auto& [id,_]:m_impl->nodes) if(!IsActive(pid,id)&&CanSpend(pid,id)) v.push_back(id); return v;
}
std::vector<std::string> TalentTree::GetActiveNodes(uint32_t pid) const {
    std::vector<std::string> v; for(auto& [id,rk]:m_impl->ranks[pid]) if(rk>0) v.push_back(id); return v;
}
void TalentTree::SetOnActivate(std::function<void(uint32_t,const std::string&,uint32_t)> cb){ m_impl->onActivate=cb; }

std::string TalentTree::SaveState(uint32_t pid) const {
    std::ostringstream ss; ss<<"{";
    bool f=true;
    for(auto& [id,rk]:m_impl->ranks[pid]){ if(!f) ss<<","; f=false; ss<<"\""<<id<<"\":"<<rk; }
    ss<<"}"; return ss.str();
}
void TalentTree::LoadState(uint32_t pid, const std::string& json){
    m_impl->ranks[pid].clear();
    size_t pos=0;
    while((pos=json.find('"',pos))!=std::string::npos){
        size_t e=json.find('"',pos+1); if(e==std::string::npos) break;
        std::string key=json.substr(pos+1,e-pos-1); pos=e+1;
        size_t c=json.find(':',pos); if(c==std::string::npos) break;
        size_t ne=json.find_first_of(",}",c+1);
        m_impl->ranks[pid][key]=(uint32_t)std::stoi(json.substr(c+1,ne-c-1)); pos=ne;
    }
}
void TalentTree::ResetPlayer(uint32_t pid){ Respec(pid); m_impl->totalPoints.erase(pid); }

} // namespace Runtime
