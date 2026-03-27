#include "Runtime/Session/SessionManager/SessionManager.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct Player {
    uint32_t    id;
    std::string displayName;
    std::unordered_map<std::string,std::string> data;
};

struct SessionData {
    uint32_t     id;
    uint32_t     maxPlayers{16};
    SessionState state{SessionState::Idle};
    std::unordered_map<uint32_t,Player> players;
    std::unordered_map<std::string,std::string> kvData;

    std::function<void(SessionState,SessionState)> onChange;
    std::function<void(uint32_t)>                  onJoin;
    std::function<void(uint32_t)>                  onLeave;
};

struct SessionManager::Impl {
    std::unordered_map<uint32_t,SessionData> sessions;
    SessionData* Find(uint32_t id){ auto it=sessions.find(id); return it!=sessions.end()?&it->second:nullptr; }
    void ChangeState(SessionData& s, SessionState next){
        SessionState prev=s.state; s.state=next;
        if(s.onChange) s.onChange(prev,next);
    }
};

SessionManager::SessionManager(): m_impl(new Impl){}
SessionManager::~SessionManager(){ Shutdown(); delete m_impl; }
void SessionManager::Init(){}
void SessionManager::Shutdown(){ m_impl->sessions.clear(); }
void SessionManager::Reset(){ m_impl->sessions.clear(); }

bool SessionManager::CreateSession(uint32_t id, uint32_t maxP){
    if(m_impl->sessions.count(id)) return false;
    SessionData s; s.id=id; s.maxPlayers=maxP;
    m_impl->sessions[id]=s; return true;
}
void SessionManager::DestroySession(uint32_t id){ m_impl->sessions.erase(id); }

bool SessionManager::JoinSession(uint32_t sessionId,uint32_t playerId,const std::string& name){
    auto* s=m_impl->Find(sessionId); if(!s) return false;
    if(s->players.size()>=s->maxPlayers) return false;
    Player p; p.id=playerId; p.displayName=name;
    s->players[playerId]=p;
    if(s->onJoin) s->onJoin(playerId);
    return true;
}
void SessionManager::LeaveSession(uint32_t sessionId,uint32_t playerId){
    auto* s=m_impl->Find(sessionId); if(!s) return;
    if(s->players.erase(playerId)&&s->onLeave) s->onLeave(playerId);
}
uint32_t SessionManager::GetPlayerIds(uint32_t sessionId,std::vector<uint32_t>& out) const {
    out.clear(); auto* s=m_impl->Find(sessionId); if(!s) return 0;
    for(auto& [id,p]:s->players) out.push_back(id);
    return (uint32_t)out.size();
}
uint32_t SessionManager::GetPlayerCount(uint32_t sessionId) const {
    auto* s=m_impl->Find(sessionId); return s?(uint32_t)s->players.size():0;
}
std::string SessionManager::GetDisplayName(uint32_t sid,uint32_t pid) const {
    auto* s=m_impl->Find(sid); if(!s) return "";
    auto it=s->players.find(pid); return it!=s->players.end()?it->second.displayName:"";
}

void SessionManager::SetState(uint32_t id,SessionState st){
    auto* s=m_impl->Find(id); if(s) m_impl->ChangeState(*s,st);
}
SessionState SessionManager::GetState(uint32_t id) const {
    auto* s=m_impl->Find(id); return s?s->state:SessionState::Idle;
}
void SessionManager::StartSession(uint32_t id){
    auto* s=m_impl->Find(id); if(s) m_impl->ChangeState(*s,SessionState::Active);
}
void SessionManager::PauseSession(uint32_t id){
    auto* s=m_impl->Find(id); if(s) m_impl->ChangeState(*s,SessionState::Paused);
}
void SessionManager::EndSession(uint32_t id){
    auto* s=m_impl->Find(id); if(s) m_impl->ChangeState(*s,SessionState::Ended);
}

void SessionManager::SetPlayerData(uint32_t sid,uint32_t pid,const std::string& k,const std::string& v){
    auto* s=m_impl->Find(sid); if(!s) return;
    auto it=s->players.find(pid); if(it!=s->players.end()) it->second.data[k]=v;
}
std::string SessionManager::GetPlayerData(uint32_t sid,uint32_t pid,const std::string& k,const std::string& def) const {
    auto* s=m_impl->Find(sid); if(!s) return def;
    auto it=s->players.find(pid); if(it==s->players.end()) return def;
    auto kit=it->second.data.find(k); return kit!=it->second.data.end()?kit->second:def;
}
void SessionManager::SetSessionData(uint32_t sid,const std::string& k,const std::string& v){
    auto* s=m_impl->Find(sid); if(s) s->kvData[k]=v;
}
std::string SessionManager::GetSessionData(uint32_t sid,const std::string& k,const std::string& def) const {
    auto* s=m_impl->Find(sid); if(!s) return def;
    auto it=s->kvData.find(k); return it!=s->kvData.end()?it->second:def;
}

void SessionManager::SetOnStateChange(uint32_t id,std::function<void(SessionState,SessionState)> cb){
    auto* s=m_impl->Find(id); if(s) s->onChange=cb;
}
void SessionManager::SetOnPlayerJoin(uint32_t id,std::function<void(uint32_t)> cb){
    auto* s=m_impl->Find(id); if(s) s->onJoin=cb;
}
void SessionManager::SetOnPlayerLeave(uint32_t id,std::function<void(uint32_t)> cb){
    auto* s=m_impl->Find(id); if(s) s->onLeave=cb;
}
uint32_t SessionManager::GetActiveSessions(std::vector<uint32_t>& out) const {
    out.clear();
    for(auto& [id,s]:m_impl->sessions)
        if(s.state==SessionState::Active||s.state==SessionState::Lobby)
            out.push_back(id);
    return (uint32_t)out.size();
}

} // namespace Runtime
