#include "Runtime/Narrative/NarrativeManager/NarrativeManager.h"
#include <algorithm>
#include <sstream>

namespace Runtime {

struct NarrativeManager::Impl {
    std::unordered_map<std::string, NarrativeBeat> beats;
    std::string currentId;
    std::unordered_map<std::string,std::string> flags;
    std::vector<JournalEntry> journal;
    uint32_t tick{0};
    std::function<void(const NarrativeBeat&)> onEnter;
    std::function<void(const std::string&)>   onExit;

    bool EvalCondition(const std::string& cond) const {
        if(cond.empty()) return true;
        auto eq = cond.find("==");
        if(eq==std::string::npos) return true;
        std::string name=cond.substr(0,eq), val=cond.substr(eq+2);
        auto it=flags.find(name);
        return it!=flags.end() && it->second==val;
    }
};

NarrativeManager::NarrativeManager(): m_impl(new Impl){}
NarrativeManager::~NarrativeManager(){ Shutdown(); delete m_impl; }
void NarrativeManager::Init(){}
void NarrativeManager::Shutdown(){ m_impl->beats.clear(); m_impl->journal.clear(); m_impl->flags.clear(); }
void NarrativeManager::Reset(){ m_impl->flags.clear(); m_impl->journal.clear(); m_impl->currentId.clear(); m_impl->tick=0; }

void NarrativeManager::AddBeat(const NarrativeBeat& b){ m_impl->beats[b.id]=b; }
void NarrativeManager::RemoveBeat(const std::string& id){ m_impl->beats.erase(id); }

void NarrativeManager::StartBeat(const std::string& id){
    auto it=m_impl->beats.find(id);
    if(it==m_impl->beats.end()) return;
    if(!m_impl->currentId.empty() && m_impl->onExit) m_impl->onExit(m_impl->currentId);
    m_impl->currentId=id;
    if(it->second.journalEntry){
        m_impl->journal.push_back({id, it->second.body, m_impl->tick});
    }
    if(m_impl->onEnter) m_impl->onEnter(it->second);
}

void NarrativeManager::Choose(uint32_t idx){
    auto* beat=GetCurrentBeat();
    if(!beat) return;
    auto available=GetAvailableChoices();
    if(idx>=available.size()) return;
    const NarrativeChoice* ch=available[idx];
    if(m_impl->onExit) m_impl->onExit(m_impl->currentId);
    if(ch->nextBeatId.empty()){ m_impl->currentId.clear(); return; }
    StartBeat(ch->nextBeatId);
}

bool NarrativeManager::IsAtEnd() const {
    auto* b=GetCurrentBeat();
    if(!b) return true;
    return GetAvailableChoices().empty();
}

const NarrativeBeat* NarrativeManager::GetCurrentBeat() const {
    auto it=m_impl->beats.find(m_impl->currentId);
    return (it!=m_impl->beats.end())? &it->second : nullptr;
}

std::vector<const NarrativeChoice*> NarrativeManager::GetAvailableChoices() const {
    std::vector<const NarrativeChoice*> out;
    auto* b=GetCurrentBeat();
    if(!b) return out;
    for(auto& c : b->choices)
        if(m_impl->EvalCondition(c.condition))
            out.push_back(&c);
    return out;
}

const std::string& NarrativeManager::GetCurrentBeatId() const { return m_impl->currentId; }

void NarrativeManager::SetFlag(const std::string& n, const std::string& v){ m_impl->flags[n]=v; }
std::string NarrativeManager::GetFlag(const std::string& n) const {
    auto it=m_impl->flags.find(n); return it!=m_impl->flags.end()?it->second:"";
}
bool NarrativeManager::HasFlag(const std::string& n) const { return m_impl->flags.count(n)>0; }
void NarrativeManager::ClearFlag(const std::string& n){ m_impl->flags.erase(n); }

const std::vector<NarrativeManager::JournalEntry>& NarrativeManager::GetJournal() const { return m_impl->journal; }
void NarrativeManager::ClearJournal(){ m_impl->journal.clear(); }

void NarrativeManager::Tick(float dt){ m_impl->tick++; (void)dt; }

void NarrativeManager::SetOnBeatEnter(std::function<void(const NarrativeBeat&)> cb){ m_impl->onEnter=cb; }
void NarrativeManager::SetOnBeatExit (std::function<void(const std::string&)> cb)  { m_impl->onExit=cb; }

std::string NarrativeManager::SaveToJSON() const {
    std::ostringstream os;
    os<<"{\"currentId\":\""<<m_impl->currentId<<"\",\"flags\":{";
    bool first=true;
    for(auto& [k,v]:m_impl->flags){ if(!first)os<<","; os<<"\""<<k<<"\":\""<<v<<"\""; first=false; }
    os<<"}}";
    return os.str();
}

bool NarrativeManager::LoadFromJSON(const std::string& /*json*/){ return true; /* stub */ }

} // namespace Runtime
