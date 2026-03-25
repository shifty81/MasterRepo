#include "Runtime/Factions/FactionSystem.h"
#include <algorithm>
#include <stdexcept>

namespace Runtime {

void FactionSystem::Init() {
    m_factions.clear();
    m_reputation.clear();
    m_diplomacy.clear();
    m_history.clear();
    m_nextId = 1;
}

// ── Factions ──────────────────────────────────────────────────────────────────

uint32_t FactionSystem::CreateFaction(const std::string& name,
                                       FactionAlignment alignment,
                                       const std::string& description,
                                       bool playerJoinable) {
    Faction f;
    f.id               = m_nextId++;
    f.name             = name;
    f.defaultAlignment = alignment;
    f.description      = description;
    f.playerJoinable   = playerJoinable;
    uint32_t id        = f.id;
    m_factions[id]     = std::move(f);
    m_reputation[id]   = 0.0f;
    return id;
}

void FactionSystem::DestroyFaction(uint32_t factionId) {
    m_factions.erase(factionId);
    m_reputation.erase(factionId);
}

Faction* FactionSystem::GetFaction(uint32_t factionId) {
    auto it = m_factions.find(factionId);
    return it != m_factions.end() ? &it->second : nullptr;
}
const Faction* FactionSystem::GetFaction(uint32_t factionId) const {
    auto it = m_factions.find(factionId);
    return it != m_factions.end() ? &it->second : nullptr;
}

std::vector<uint32_t> FactionSystem::AllFactionIds() const {
    std::vector<uint32_t> ids;
    for (const auto& [id, _] : m_factions) ids.push_back(id);
    return ids;
}

// ── Player reputation ─────────────────────────────────────────────────────────

void FactionSystem::AddReputation(uint32_t factionId, float delta,
                                   const std::string& reason) {
    auto it = m_reputation.find(factionId);
    if (it == m_reputation.end()) return;
    it->second = std::clamp(it->second + delta, -1000.0f, 1000.0f);
    ReputationEvent ev{factionId, delta, reason};
    m_history.push_back(ev);
    if (m_onRepChanged) m_onRepChanged(ev);
}

float FactionSystem::GetReputation(uint32_t factionId) const {
    auto it = m_reputation.find(factionId);
    return it != m_reputation.end() ? it->second : 0.0f;
}

void FactionSystem::SetReputation(uint32_t factionId, float value) {
    m_reputation[factionId] = std::clamp(value, -1000.0f, 1000.0f);
}

PlayerStance FactionSystem::GetStance(uint32_t factionId) const {
    float rep = GetReputation(factionId);
    if (rep >= kAlliedThreshold)   return PlayerStance::Allied;
    if (rep >= kFriendlyThreshold) return PlayerStance::Friendly;
    if (rep >= kNeutralThreshold)  return PlayerStance::Neutral;
    if (rep >= kCautiousThreshold) return PlayerStance::Cautious;
    return PlayerStance::Hostile;
}

// ── Diplomacy ─────────────────────────────────────────────────────────────────

uint32_t FactionSystem::DiplomacyKey(uint32_t a, uint32_t b) const {
    // Encode two 16-bit IDs into a 32-bit key (order-independent)
    if (a > b) std::swap(a, b);
    return (a << 16) | (b & 0xFFFF);
}

void FactionSystem::SetDiplomacy(uint32_t factionA, uint32_t factionB,
                                  DiplomaticState state) {
    uint32_t key = DiplomacyKey(factionA, factionB);
    m_diplomacy[key] = state;
    if (m_onDipChanged) m_onDipChanged(factionA, factionB, state);
}

DiplomaticState FactionSystem::GetDiplomacy(uint32_t factionA, uint32_t factionB) const {
    uint32_t key = DiplomacyKey(factionA, factionB);
    auto it = m_diplomacy.find(key);
    return it != m_diplomacy.end() ? it->second : DiplomaticState::Neutral;
}

bool FactionSystem::IsAtWar(uint32_t factionA, uint32_t factionB) const {
    return GetDiplomacy(factionA, factionB) == DiplomaticState::War;
}

bool FactionSystem::IsAllied(uint32_t factionA, uint32_t factionB) const {
    return GetDiplomacy(factionA, factionB) == DiplomaticState::Alliance;
}

// ── Cross-faction reputation propagation ──────────────────────────────────────

void FactionSystem::PropagateHostileAct(uint32_t targetFactionId, float baseDelta) {
    // The target faction itself takes the full hit
    AddReputation(targetFactionId, baseDelta, "hostile_act");
    // Allied factions take half the hit
    for (const auto& [id, _] : m_factions) {
        if (id == targetFactionId) continue;
        if (IsAllied(id, targetFactionId))
            AddReputation(id, baseDelta * 0.5f, "allied_hostile_act");
    }
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void FactionSystem::SetReputationChangedCallback(ReputationChangedFn fn) {
    m_onRepChanged = std::move(fn);
}
void FactionSystem::SetDiplomacyChangedCallback(DiplomacyChangedFn fn) {
    m_onDipChanged = std::move(fn);
}

// ── Stats ─────────────────────────────────────────────────────────────────────

size_t FactionSystem::FactionCount() const { return m_factions.size(); }
const std::vector<ReputationEvent>& FactionSystem::GetReputationHistory() const { return m_history; }
void FactionSystem::ClearHistory() { m_history.clear(); }

} // namespace Runtime
