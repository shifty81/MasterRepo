#include "Runtime/Gameplay/AchievementSystem/AchievementSystem.h"
#include <algorithm>
#include <chrono>
#include <sstream>

namespace Runtime::Gameplay {

static uint64_t AchNow() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()).count());
}

struct AchievementSystem::Impl {
    std::unordered_map<std::string, AchievementDef>   defs;
    std::unordered_map<std::string, AchievementState> states;
    std::vector<UnlockCallback>   unlockCbs;
    std::vector<ProgressCallback> progressCbs;
};

AchievementSystem::AchievementSystem() : m_impl(new Impl()) {}
AchievementSystem::~AchievementSystem() { delete m_impl; }

void AchievementSystem::Register(const AchievementDef& def) {
    m_impl->defs[def.id] = def;
    // Ensure a state entry exists
    auto& st = m_impl->states[def.id];
    st.id     = def.id;
    st.target = def.targetValue;
}

bool AchievementSystem::IsRegistered(const std::string& id) const {
    return m_impl->defs.count(id) > 0;
}

const AchievementDef* AchievementSystem::GetDef(const std::string& id) const {
    auto it = m_impl->defs.find(id);
    return it != m_impl->defs.end() ? &it->second : nullptr;
}

std::vector<std::string> AchievementSystem::AllIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_impl->defs.size());
    for (const auto& [k,_] : m_impl->defs) ids.push_back(k);
    std::sort(ids.begin(), ids.end());
    return ids;
}

size_t AchievementSystem::Count() const { return m_impl->defs.size(); }

void AchievementSystem::unlock(AchievementState& st, const AchievementDef& def) {
    if (st.unlocked) return;
    st.unlocked     = true;
    st.unlockedAtMs = AchNow();
    UnlockEvent ev;
    ev.id           = def.id;
    ev.name         = def.name;
    ev.rewardXP     = def.rewardXP;
    ev.rewardItemId = def.rewardItemId;
    ev.timestampMs  = st.unlockedAtMs;
    for (auto& cb : m_impl->unlockCbs) cb(ev);
}

void AchievementSystem::Update(const std::string& eventKey, float delta) {
    for (auto& [id, def] : m_impl->defs) {
        if (def.eventKey != eventKey) continue;
        auto& st = m_impl->states[id];
        if (st.unlocked) continue;

        switch (def.condition) {
        case AchievementCondition::Cumulative:
        case AchievementCondition::TimedAccumulation:
            if (delta > 0.0f) {
                st.progress += delta;
                for (auto& cb : m_impl->progressCbs) cb(id, st.progress, st.target);
                if (st.progress >= st.target) unlock(st, def);
            }
            break;

        case AchievementCondition::SingleEvent:
            if (delta >= def.targetValue) unlock(st, def);
            else {
                st.progress = delta;
                for (auto& cb : m_impl->progressCbs) cb(id, st.progress, st.target);
            }
            break;

        case AchievementCondition::Streak:
            if (delta > 0.0f) {
                ++st.streakCurrent;
                if (st.streakCurrent > st.streakBest) st.streakBest = st.streakCurrent;
                st.progress = static_cast<float>(st.streakCurrent);
                for (auto& cb : m_impl->progressCbs) cb(id, st.progress, st.target);
                if (st.streakCurrent >= static_cast<uint32_t>(def.targetValue))
                    unlock(st, def);
            } else {
                st.streakCurrent = 0;
                st.progress = 0.0f;
            }
            break;
        }
    }
}

void AchievementSystem::ForceUnlock(const std::string& id) {
    auto dit = m_impl->defs.find(id);
    if (dit == m_impl->defs.end()) return;
    auto& st = m_impl->states[id];
    unlock(st, dit->second);
}

void AchievementSystem::ResetStreak(const std::string& eventKey) {
    for (auto& [id, def] : m_impl->defs) {
        if (def.eventKey != eventKey) continue;
        if (def.condition != AchievementCondition::Streak) continue;
        auto& st = m_impl->states[id];
        st.streakCurrent = 0;
        st.progress = 0.0f;
    }
}

const AchievementState* AchievementSystem::GetState(const std::string& id) const {
    auto it = m_impl->states.find(id);
    return it != m_impl->states.end() ? &it->second : nullptr;
}
bool AchievementSystem::IsUnlocked(const std::string& id) const {
    auto* s = GetState(id); return s && s->unlocked;
}
float AchievementSystem::GetProgress(const std::string& id) const {
    auto* s = GetState(id); return s ? s->progress : 0.0f;
}

std::vector<AchievementState> AchievementSystem::UnlockedAchievements() const {
    std::vector<AchievementState> out;
    for (const auto& [_,st] : m_impl->states) if (st.unlocked) out.push_back(st);
    return out;
}
std::vector<AchievementState> AchievementSystem::InProgress() const {
    std::vector<AchievementState> out;
    for (const auto& [_,st] : m_impl->states)
        if (!st.unlocked && st.progress > 0.0f) out.push_back(st);
    return out;
}
size_t AchievementSystem::UnlockedCount() const {
    size_t n = 0;
    for (const auto& [_,st] : m_impl->states) if (st.unlocked) ++n;
    return n;
}

void AchievementSystem::OnUnlock(UnlockCallback cb)   { m_impl->unlockCbs.push_back(std::move(cb)); }
void AchievementSystem::OnProgress(ProgressCallback cb){ m_impl->progressCbs.push_back(std::move(cb)); }

std::unordered_map<std::string,std::string> AchievementSystem::ExportState() const {
    std::unordered_map<std::string,std::string> out;
    for (const auto& [id, st] : m_impl->states) {
        out[id + ".progress"]  = std::to_string(st.progress);
        out[id + ".unlocked"]  = st.unlocked ? "1" : "0";
        out[id + ".streak"]    = std::to_string(st.streakCurrent);
        out[id + ".bestStreak"]= std::to_string(st.streakBest);
        out[id + ".ts"]        = std::to_string(st.unlockedAtMs);
    }
    return out;
}

void AchievementSystem::ImportState(const std::unordered_map<std::string,std::string>& data) {
    for (const auto& [key, val] : data) {
        auto dot = key.rfind('.');
        if (dot == std::string::npos) continue;
        std::string id  = key.substr(0, dot);
        std::string fld = key.substr(dot + 1);
        if (!m_impl->defs.count(id)) continue;
        auto& st = m_impl->states[id];
        st.id = id;
        try {
            if (fld == "progress")   st.progress        = std::stof(val);
            else if (fld == "unlocked") st.unlocked      = (val == "1");
            else if (fld == "streak")   st.streakCurrent = static_cast<uint32_t>(std::stoul(val));
            else if (fld == "bestStreak") st.streakBest  = static_cast<uint32_t>(std::stoul(val));
            else if (fld == "ts")       st.unlockedAtMs  = std::stoull(val);
        } catch (...) {}
    }
    // Sync targets
    for (auto& [id, st] : m_impl->states)
        if (m_impl->defs.count(id)) st.target = m_impl->defs[id].targetValue;
}

void AchievementSystem::Reset() {
    for (auto& [id, st] : m_impl->states) {
        st.progress      = 0.0f;
        st.unlocked      = false;
        st.unlockedAtMs  = 0;
        st.streakCurrent = 0;
        st.streakBest    = 0;
    }
}

} // namespace Runtime::Gameplay
