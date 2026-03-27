#include "Runtime/Achievement/AchievementSystem/AchievementSystem.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct AchievementSystem::Impl {
    std::vector<AchievementDef>   defs;
    std::vector<AchievementState> states;

    std::unordered_map<std::string,uint32_t>                          stats;
    std::vector<std::tuple<std::string,std::string,uint32_t>>         statBindings; // statKey, achievId, threshold

    UnlockCb                                                          onUnlock;
    std::function<void(const AchievementDef&, uint32_t)>              onProgress;
    PlatformUnlockFn                                                  platformFn;

    AchievementDef* FindDef(const std::string& id) {
        for (auto& d : defs) if (d.id==id) return &d;
        return nullptr;
    }
    AchievementState* FindState(const std::string& id) {
        for (auto& s : states) if (s.id==id) return &s;
        return nullptr;
    }
    const AchievementDef*   FindDef  (const std::string& id) const {
        for (auto& d : defs)   if (d.id==id) return &d;   return nullptr; }
    const AchievementState* FindState(const std::string& id) const {
        for (auto& s : states) if (s.id==id) return &s; return nullptr; }

    void DoUnlock(const std::string& id) {
        auto* s = FindState(id); if(!s||s->unlocked) return;
        auto* d = FindDef(id);   if(!d) return;
        s->unlocked = true;
        s->progress = d->progressGoal;
        s->unlockTimestamp = (uint64_t)std::chrono::system_clock::now().time_since_epoch().count();
        if (onUnlock) onUnlock(*d);
        if (platformFn) platformFn(id);
    }
};

AchievementSystem::AchievementSystem()  : m_impl(new Impl) {}
AchievementSystem::~AchievementSystem() { Shutdown(); delete m_impl; }

void AchievementSystem::Init()     {}
void AchievementSystem::Shutdown() { m_impl->defs.clear(); m_impl->states.clear(); }

void AchievementSystem::Register(const AchievementDef& def)
{
    m_impl->defs.push_back(def);
    AchievementState s; s.id=def.id;
    m_impl->states.push_back(s);
}

bool AchievementSystem::Has(const std::string& id) const { return m_impl->FindDef(id)!=nullptr; }
const AchievementDef* AchievementSystem::Get(const std::string& id) const { return m_impl->FindDef(id); }

std::vector<AchievementDef> AchievementSystem::GetAll() const { return m_impl->defs; }

std::vector<AchievementDef> AchievementSystem::GetByCategory(const std::string& cat) const
{
    std::vector<AchievementDef> out;
    for (auto& d : m_impl->defs) if (d.category==cat) out.push_back(d);
    return out;
}

void AchievementSystem::Unlock(const std::string& id) { m_impl->DoUnlock(id); }

void AchievementSystem::SetProgress(const std::string& id, uint32_t progress)
{
    auto* s = m_impl->FindState(id); if (!s||s->unlocked) return;
    auto* d = m_impl->FindDef(id);   if (!d) return;
    s->progress = std::min(progress, d->progressGoal);
    if (m_impl->onProgress) m_impl->onProgress(*d, s->progress);
    if (s->progress >= d->progressGoal) m_impl->DoUnlock(id);
}

void AchievementSystem::AddProgress(const std::string& id, uint32_t delta)
{
    auto* s = m_impl->FindState(id); if (!s||s->unlocked) return;
    SetProgress(id, s->progress+delta);
}

bool     AchievementSystem::IsUnlocked(const std::string& id) const
{ const auto* s=m_impl->FindState(id); return s&&s->unlocked; }
uint32_t AchievementSystem::GetProgress(const std::string& id) const
{ const auto* s=m_impl->FindState(id); return s?s->progress:0; }
const AchievementState* AchievementSystem::GetState(const std::string& id) const
{ return m_impl->FindState(id); }

void AchievementSystem::IncrementStat(const std::string& key, uint32_t delta)
{
    m_impl->stats[key] += delta;
    uint32_t val = m_impl->stats[key];
    for (auto& [k,aid,thresh] : m_impl->statBindings) {
        if (k==key && val>=thresh) AddProgress(aid, 1);
    }
}

void AchievementSystem::SetStat(const std::string& key, uint32_t value)
{ m_impl->stats[key]=value; }
uint32_t AchievementSystem::GetStat(const std::string& key) const
{ auto it=m_impl->stats.find(key); return it!=m_impl->stats.end()?it->second:0u; }

void AchievementSystem::BindStatToAchievement(const std::string& statKey,
                                               const std::string& achievId,
                                               uint32_t threshold)
{
    m_impl->statBindings.emplace_back(statKey, achievId, threshold);
}

uint32_t AchievementSystem::TotalUnlocked() const
{
    uint32_t n=0; for(auto& s:m_impl->states) if(s.unlocked) n++; return n;
}
uint32_t AchievementSystem::TotalPoints()  const
{ uint32_t n=0; for(auto& d:m_impl->defs) n+=d.points; return n; }
uint32_t AchievementSystem::EarnedPoints() const
{
    uint32_t n=0;
    for(auto& s:m_impl->states) if(s.unlocked){
        auto* d=m_impl->FindDef(s.id); if(d) n+=d->points;
    }
    return n;
}
float AchievementSystem::CompletionPct() const
{
    if (m_impl->defs.empty()) return 0.f;
    return 100.f * TotalUnlocked() / (float)m_impl->defs.size();
}

void AchievementSystem::SetOnUnlock(UnlockCb cb) { m_impl->onUnlock=cb; }
void AchievementSystem::SetOnProgress(std::function<void(const AchievementDef&, uint32_t)> cb)
{ m_impl->onProgress=cb; }
void AchievementSystem::SetPlatformUnlockFn(PlatformUnlockFn fn) { m_impl->platformFn=fn; }

std::string AchievementSystem::Serialize() const
{
    std::ostringstream ss;
    ss << "{\"achievements\":[";
    bool first=true;
    for (auto& s : m_impl->states) {
        if (!first) ss<<","; first=false;
        ss << "{\"id\":\"" << s.id << "\","
           << "\"unlocked\":" << (s.unlocked?"true":"false") << ","
           << "\"progress\":" << s.progress << "}";
    }
    ss << "]}";
    return ss.str();
}

bool AchievementSystem::Deserialize(const std::string& /*json*/) { return true; }

void AchievementSystem::Tick(float /*dt*/) {}

} // namespace Runtime
