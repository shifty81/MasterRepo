#include "Core/Events/GameEventBus/GameEventBus.h"
#include <algorithm>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

namespace Core {

// ── GameEvent accessors ───────────────────────────────────────────────────────

bool        GameEvent::GetBool(const std::string& k, bool def) const {
    auto it=data.find(k); if(it==data.end()) return def;
    if(auto* p=std::get_if<bool>(&it->second)) return *p;
    return def;
}
int32_t     GameEvent::GetInt(const std::string& k, int32_t def) const {
    auto it=data.find(k); if(it==data.end()) return def;
    if(auto* p=std::get_if<int32_t>(&it->second)) return *p;
    return def;
}
float       GameEvent::GetFloat(const std::string& k, float def) const {
    auto it=data.find(k); if(it==data.end()) return def;
    if(auto* p=std::get_if<float>(&it->second)) return *p;
    return def;
}
std::string GameEvent::GetString(const std::string& k, const std::string& def) const {
    auto it=data.find(k); if(it==data.end()) return def;
    if(auto* p=std::get_if<std::string>(&it->second)) return *p;
    return def;
}

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct Subscription {
    SubscribeToken token{0};
    int32_t        priority{0};
    bool           oneShot{false};
    EventHandler   handler;
    EventFilter    filter;
    bool           dead{false};
};

struct GameEventBus::Impl {
    std::unordered_map<std::string, std::vector<Subscription>> subs;
    SubscribeToken nextToken{1};
    std::queue<GameEvent> deferred;
    std::mutex deferredMutex;
    uint64_t frame{0};

    void Dispatch(const GameEvent& evt) {
        auto it=subs.find(evt.channel); if(it==subs.end()) return;
        auto& list=it->second;
        // Sort by priority descending (lazy)
        std::sort(list.begin(),list.end(),[](auto& a,auto& b){ return a.priority>b.priority; });
        for(auto& s: list) {
            if(s.dead) continue;
            if(s.filter && !s.filter(evt)) continue;
            s.handler(evt);
            if(s.oneShot) s.dead=true;
        }
        // Prune dead
        list.erase(std::remove_if(list.begin(),list.end(),[](auto& s){ return s.dead; }),list.end());
    }
};

GameEventBus::GameEventBus() : m_impl(new Impl()) {}
GameEventBus::~GameEventBus() { delete m_impl; }
void GameEventBus::Init()     {}
void GameEventBus::Shutdown() { m_impl->subs.clear(); }

SubscribeToken GameEventBus::Subscribe(const std::string& ch, EventHandler handler,
                                        int32_t priority, EventFilter filter) {
    SubscribeToken tok=m_impl->nextToken++;
    Subscription s; s.token=tok; s.priority=priority; s.handler=std::move(handler); s.filter=std::move(filter);
    m_impl->subs[ch].push_back(std::move(s));
    return tok;
}
SubscribeToken GameEventBus::SubscribeOnce(const std::string& ch, EventHandler handler, EventFilter filter) {
    SubscribeToken tok=m_impl->nextToken++;
    Subscription s; s.token=tok; s.oneShot=true; s.handler=std::move(handler); s.filter=std::move(filter);
    m_impl->subs[ch].push_back(std::move(s));
    return tok;
}
void GameEventBus::Unsubscribe(SubscribeToken tok) {
    for(auto& [ch,list]: m_impl->subs)
        for(auto& s: list) if(s.token==tok) { s.dead=true; return; }
}
void GameEventBus::UnsubscribeAll(const std::string& ch){ m_impl->subs.erase(ch); }

void GameEventBus::Post(const GameEvent& evt) {
    GameEvent e=evt; e.timestamp=m_impl->frame;
    m_impl->Dispatch(e);
}
void GameEventBus::Post(const std::string& ch){ Post(GameEvent(ch)); }

void GameEventBus::PostDeferred(const GameEvent& evt) {
    GameEvent e=evt; e.timestamp=m_impl->frame;
    std::lock_guard<std::mutex> g(m_impl->deferredMutex);
    m_impl->deferred.push(e);
}
void GameEventBus::FlushDeferred() {
    std::queue<GameEvent> local;
    { std::lock_guard<std::mutex> g(m_impl->deferredMutex); std::swap(local,m_impl->deferred); }
    while(!local.empty()) { m_impl->Dispatch(local.front()); local.pop(); }
}

uint32_t GameEventBus::SubscriberCount(const std::string& ch) const {
    auto it=m_impl->subs.find(ch); return it!=m_impl->subs.end()?(uint32_t)it->second.size():0;
}
uint32_t GameEventBus::PendingDeferredCount() const {
    std::lock_guard<std::mutex> g(m_impl->deferredMutex);
    return (uint32_t)m_impl->deferred.size();
}
void GameEventBus::SetFrameCounter(uint64_t f){ m_impl->frame=f; }

} // namespace Core
