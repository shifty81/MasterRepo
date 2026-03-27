#include "Runtime/Leaderboard/LeaderboardSystem/LeaderboardSystem.h"
#include <algorithm>
#include <cstdio>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct LeaderboardSystem::Impl {
    std::vector<LeaderboardEntry> entries;
    bool     descending{true};
    std::string tieBreaker{"submission_order"};
    uint32_t maxEntries{UINT32_MAX};
    uint64_t submissionCounter{0};
    std::vector<std::pair<uint32_t, std::function<void(const LeaderboardEntry&)>>> topCallbacks;

    bool CompareEntries(const LeaderboardEntry& a, const LeaderboardEntry& b) const {
        // Primary: score
        if(a.score!=b.score) return descending?(a.score>b.score):(a.score<b.score);
        // Tie-break
        if(tieBreaker=="alphabetical") return a.playerName<b.playerName;
        // Default: earlier submission wins
        return a.submissionOrder<b.submissionOrder;
    }

    void Rebuild(){
        std::sort(entries.begin(),entries.end(),
            [this](const LeaderboardEntry& a,const LeaderboardEntry& b){
                return CompareEntries(a,b);
            });
        for(uint32_t i=0;i<(uint32_t)entries.size();i++) entries[i].rank=i+1;
        // Trim
        if(entries.size()>maxEntries) entries.resize(maxEntries);
    }
};

LeaderboardSystem::LeaderboardSystem(): m_impl(new Impl){}
LeaderboardSystem::~LeaderboardSystem(){ Shutdown(); delete m_impl; }
void LeaderboardSystem::Init(){}
void LeaderboardSystem::Shutdown(){ m_impl->entries.clear(); }
void LeaderboardSystem::Reset(){ m_impl->entries.clear(); m_impl->submissionCounter=0; }
void LeaderboardSystem::Clear(){ Reset(); }

void LeaderboardSystem::SubmitScore(uint32_t playerId, const std::string& name,
                                     float score, const std::string& metadata){
    // Update or insert
    for(auto& e:m_impl->entries){
        if(e.playerId==playerId){
            bool improve=m_impl->descending?(score>e.score):(score<e.score);
            if(improve){ e.score=score; e.playerName=name; e.metadata=metadata;
                         e.submissionOrder=m_impl->submissionCounter++; }
            m_impl->Rebuild(); return;
        }
    }
    LeaderboardEntry ne; ne.playerId=playerId; ne.playerName=name;
    ne.score=score; ne.metadata=metadata; ne.submissionOrder=m_impl->submissionCounter++;
    m_impl->entries.push_back(ne);
    m_impl->Rebuild();

    // Top-N callbacks
    auto* ep=GetEntry(playerId);
    if(ep){
        for(auto& [topN,cb]:m_impl->topCallbacks)
            if(ep->rank<=topN && cb) cb(*ep);
    }
}

uint32_t LeaderboardSystem::GetRank(uint32_t playerId) const {
    auto* e=GetEntry(playerId); return e?e->rank:UINT32_MAX;
}

std::vector<LeaderboardEntry> LeaderboardSystem::GetTopN(uint32_t n) const {
    std::vector<LeaderboardEntry> out;
    for(uint32_t i=0;i<std::min(n,(uint32_t)m_impl->entries.size());i++)
        out.push_back(m_impl->entries[i]);
    return out;
}

std::vector<LeaderboardEntry> LeaderboardSystem::GetPage(uint32_t pageIdx, uint32_t pageSize) const {
    std::vector<LeaderboardEntry> out;
    uint32_t start=pageIdx*pageSize;
    for(uint32_t i=start;i<std::min(start+pageSize,(uint32_t)m_impl->entries.size());i++)
        out.push_back(m_impl->entries[i]);
    return out;
}

const LeaderboardEntry* LeaderboardSystem::GetEntry(uint32_t playerId) const {
    for(auto& e:m_impl->entries) if(e.playerId==playerId) return &e;
    return nullptr;
}

uint32_t LeaderboardSystem::GetTotalEntries() const {
    return (uint32_t)m_impl->entries.size();
}

void LeaderboardSystem::SetSortDescending(bool on){ m_impl->descending=on; m_impl->Rebuild(); }
void LeaderboardSystem::SetTieBreaker    (const std::string& f){ m_impl->tieBreaker=f; m_impl->Rebuild(); }
void LeaderboardSystem::SetMaxEntries    (uint32_t n){ m_impl->maxEntries=n; m_impl->Rebuild(); }

bool LeaderboardSystem::Save(const std::string& path) const {
    FILE* f=fopen(path.c_str(),"w"); if(!f) return false;
    fprintf(f,"[\n");
    for(size_t i=0;i<m_impl->entries.size();i++){
        auto& e=m_impl->entries[i];
        fprintf(f,"{\"id\":%u,\"name\":\"%s\",\"score\":%.6f,\"rank\":%u}%s\n",
            e.playerId, e.playerName.c_str(), e.score, e.rank,
            i+1<m_impl->entries.size()?",":"");
    }
    fprintf(f,"]\n"); fclose(f); return true;
}

bool LeaderboardSystem::Load(const std::string& path){
    // Minimal line-based parser matching Save() format
    FILE* f=fopen(path.c_str(),"r"); if(!f) return false;
    m_impl->entries.clear();
    char buf[512];
    while(fgets(buf,sizeof(buf),f)){
        uint32_t id,rank; float score; char name[256]="";
        if(sscanf(buf,"{\"id\":%u,\"name\":\"%255[^\"]\",\"score\":%f,\"rank\":%u}",
                  &id,name,&score,&rank)==4){
            SubmitScore(id,name,score);
        }
    }
    fclose(f); return true;
}

void LeaderboardSystem::SetOnNewTopEntry(uint32_t topN,
    std::function<void(const LeaderboardEntry&)> cb){
    m_impl->topCallbacks.push_back({topN,cb});
}

} // namespace Runtime
