#include "Runtime/Gameplay/ProgressionSystem.h"
#include <algorithm>
#include <cmath>

namespace Runtime {

void ProgressionSystem::Init(uint32_t maxLevel) {
    m_maxLevel     = maxLevel;
    m_level        = 1;
    m_prestigeLevel= 0;
    m_totalXP      = 0.0f;
    m_talentPoints = 0;
    m_nextTechId   = 1;
    m_tech.clear();
    m_xpMult.clear();
    m_xpHistory.clear();
}

// ── XP & Levels ───────────────────────────────────────────────────────────────

float ProgressionSystem::XPForLevel(uint32_t level) const {
    // Quadratic XP curve: level N requires N^2 * 100 total XP
    return (float)(level * level) * 100.0f;
}

void ProgressionSystem::AddXP(XPSource source, float amount, const std::string& label) {
    float mult = GetXPMultiplier(source);
    float effective = amount * mult;
    m_totalXP += effective;
    XPRecord rec{source, effective, label};
    m_xpHistory.push_back(rec);
    CheckLevelUp();
}

void ProgressionSystem::CheckLevelUp() {
    while (m_level < m_maxLevel && m_totalXP >= XPForLevel(m_level + 1)) {
        ++m_level;
        uint32_t granted = 1 + (m_level % 5 == 0 ? 1 : 0); // bonus every 5 levels
        m_talentPoints += granted;
        if (m_onLevelUp) {
            LevelUpEvent ev{ m_level, granted, m_totalXP };
            m_onLevelUp(ev);
        }
    }
}

float   ProgressionSystem::GetXP()    const { return m_totalXP; }
uint32_t ProgressionSystem::GetLevel() const { return m_level; }
uint32_t ProgressionSystem::GetMaxLevel() const { return m_maxLevel; }

float ProgressionSystem::GetXPForNextLevel() const {
    if (m_level >= m_maxLevel) return 0.0f;
    return XPForLevel(m_level + 1) - m_totalXP;
}

float ProgressionSystem::GetLevelProgress() const {
    if (m_level >= m_maxLevel) return 1.0f;
    float cur  = XPForLevel(m_level);
    float next = XPForLevel(m_level + 1);
    float span = next - cur;
    if (span <= 0.0f) return 1.0f;
    return std::clamp((m_totalXP - cur) / span, 0.0f, 1.0f);
}

// ── Talent points ─────────────────────────────────────────────────────────────

uint32_t ProgressionSystem::GetTalentPoints() const { return m_talentPoints; }

bool ProgressionSystem::SpendTalentPoint(uint32_t techNodeId) {
    auto it = m_tech.find(techNodeId);
    if (it == m_tech.end() || it->second.unlocked) return false;
    if (!CanUnlockTech(techNodeId)) return false;
    if (m_talentPoints < it->second.talentPointCost) return false;
    m_talentPoints -= it->second.talentPointCost;
    it->second.unlocked = true;
    if (m_onTechUnlocked) m_onTechUnlocked(techNodeId, it->second.name);
    return true;
}

bool ProgressionSystem::SpendTalentPoint(const std::string& techName) {
    for (auto& [id, node] : m_tech)
        if (node.name == techName) return SpendTalentPoint(id);
    return false;
}

// ── Tech tree ─────────────────────────────────────────────────────────────────

uint32_t ProgressionSystem::AddTechNode(const std::string& name,
                                          const std::string& description,
                                          uint32_t cost,
                                          const std::vector<uint32_t>& prerequisites) {
    TechNode n;
    n.id               = m_nextTechId++;
    n.name             = name;
    n.description      = description;
    n.talentPointCost  = cost;
    n.prerequisites    = prerequisites;
    uint32_t id        = n.id;
    m_tech[id]         = std::move(n);
    return id;
}

bool ProgressionSystem::IsTechUnlocked(uint32_t id) const {
    auto it = m_tech.find(id);
    return it != m_tech.end() && it->second.unlocked;
}

bool ProgressionSystem::CanUnlockTech(uint32_t id) const {
    auto it = m_tech.find(id);
    if (it == m_tech.end()) return false;
    for (uint32_t prereq : it->second.prerequisites)
        if (!IsTechUnlocked(prereq)) return false;
    return true;
}

const TechNode* ProgressionSystem::GetTechNode(uint32_t id) const {
    auto it = m_tech.find(id);
    return it != m_tech.end() ? &it->second : nullptr;
}

std::vector<uint32_t> ProgressionSystem::AllTechIds() const {
    std::vector<uint32_t> ids;
    for (const auto& [id, _] : m_tech) ids.push_back(id);
    return ids;
}

// ── Prestige ──────────────────────────────────────────────────────────────────

bool ProgressionSystem::CanPrestige() const { return m_level >= m_maxLevel; }

void ProgressionSystem::Prestige() {
    if (!CanPrestige()) return;
    ++m_prestigeLevel;
    m_level    = 1;
    m_totalXP  = 0.0f;
    // Retain tech unlocks and get 5 bonus talent points
    m_talentPoints += 5;
}

uint32_t ProgressionSystem::PrestigeLevel() const { return m_prestigeLevel; }

// ── XP multipliers ────────────────────────────────────────────────────────────

void  ProgressionSystem::SetXPMultiplier(XPSource source, float mult) {
    m_xpMult[source] = mult;
}
float ProgressionSystem::GetXPMultiplier(XPSource source) const {
    auto it = m_xpMult.find(source);
    return it != m_xpMult.end() ? it->second : 1.0f;
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void ProgressionSystem::SetLevelUpCallback(LevelUpFn fn)     { m_onLevelUp       = std::move(fn); }
void ProgressionSystem::SetTechUnlockedCallback(TechUnlockedFn fn) { m_onTechUnlocked = std::move(fn); }

// ── Stats ─────────────────────────────────────────────────────────────────────

const std::vector<XPRecord>& ProgressionSystem::GetXPHistory() const { return m_xpHistory; }
void ProgressionSystem::ClearXPHistory() { m_xpHistory.clear(); }

} // namespace Runtime
