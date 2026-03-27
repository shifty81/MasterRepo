#include "Engine/Platform/Steamworks/SteamworksStub/SteamworksStub.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Engine {

struct SteamworksStub::Impl {
    bool available{false};
    std::vector<AchievementDef>              achdefs;
    std::unordered_map<std::string,bool>     unlocked;
    std::unordered_map<std::string,int32_t>  statsInt;
    std::unordered_map<std::string,float>    statsFloat;
    std::unordered_map<std::string,uint64_t> leaderboards;
    uint64_t                                 nextLBHandle{1};
    std::function<void(const std::string&)>  onAchUnlocked;
};

SteamworksStub::SteamworksStub()  : m_impl(new Impl) {}
SteamworksStub::~SteamworksStub() { Shutdown(); delete m_impl; }

bool SteamworksStub::Init()     { m_impl->available=false; return false; }
void SteamworksStub::Shutdown() {}
void SteamworksStub::RunCallbacks() {}

bool SteamworksStub::IsAvailable() const { return m_impl->available; }

uint64_t    SteamworksStub::GetSteamID()     const { return 0; }
std::string SteamworksStub::GetPersonaName() const { return "Player"; }

void SteamworksStub::RegisterAchievement(const AchievementDef& def) {
    for(auto& a:m_impl->achdefs) if(a.id==def.id) return;
    m_impl->achdefs.push_back(def);
}
void SteamworksStub::RegisterAchievement(const std::string& id, const std::string& name,
                                           const std::string& desc, bool hidden) {
    RegisterAchievement({id,name,desc,hidden});
}

bool SteamworksStub::UnlockAchievement(const std::string& id) {
    if(m_impl->unlocked[id]) return false; // already unlocked
    m_impl->unlocked[id]=true;
    if(m_impl->onAchUnlocked) m_impl->onAchUnlocked(id);
    return true;
}
bool SteamworksStub::ClearAchievement(const std::string& id) {
    m_impl->unlocked[id]=false; return true;
}
bool SteamworksStub::IsAchieved(const std::string& id) const {
    auto it=m_impl->unlocked.find(id); return it!=m_impl->unlocked.end()&&it->second;
}
std::vector<AchievementDef> SteamworksStub::GetAchievements() const { return m_impl->achdefs; }

void SteamworksStub::SetStatInt  (const std::string& n, int32_t v) { m_impl->statsInt[n]=v; }
void SteamworksStub::SetStatFloat(const std::string& n, float v)   { m_impl->statsFloat[n]=v; }
int32_t SteamworksStub::GetStatInt  (const std::string& n, int32_t def) const {
    auto it=m_impl->statsInt.find(n); return it!=m_impl->statsInt.end()?it->second:def;
}
float SteamworksStub::GetStatFloat(const std::string& n, float def) const {
    auto it=m_impl->statsFloat.find(n); return it!=m_impl->statsFloat.end()?it->second:def;
}
bool SteamworksStub::StoreStats() { return true; }

uint64_t SteamworksStub::FindOrCreateLeaderboard(const std::string& name) {
    auto it=m_impl->leaderboards.find(name);
    if(it!=m_impl->leaderboards.end()) return it->second;
    uint64_t h=m_impl->nextLBHandle++;
    m_impl->leaderboards[name]=h; return h;
}
bool SteamworksStub::UploadScore(uint64_t /*h*/, int64_t /*score*/) { return true; }
std::vector<LeaderboardEntry> SteamworksStub::DownloadEntries(uint64_t, int32_t, int32_t) {
    return {};
}

bool SteamworksStub::IsDlcInstalled(uint32_t) const { return false; }
void SteamworksStub::ShowOverlay(const std::string&) {}

void SteamworksStub::SetOnAchievementUnlocked(std::function<void(const std::string&)> cb) {
    m_impl->onAchUnlocked=cb;
}

} // namespace Engine
