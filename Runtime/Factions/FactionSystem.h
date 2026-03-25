#pragma once
/**
 * @file FactionSystem.h
 * @brief Faction allegiances, reputation, and diplomatic states.
 *
 * Manages:
 *   - Named factions with an alignment axis (−1.0 hostile .. +1.0 friendly)
 *   - Player reputation per faction (clamped −1000 .. +1000)
 *   - Inter-faction diplomatic relations (Peace / War / Alliance / Neutral)
 *   - Reputation events: kill, quest, trade, donation, transgression
 *   - Faction AI: passive/aggressive/neutral stance per reputation threshold
 *
 * Usage:
 * @code
 *   FactionSystem factions;
 *   factions.Init();
 *   uint32_t miners = factions.CreateFaction("Miner's Guild", FactionAlignment::Neutral);
 *   uint32_t pirates = factions.CreateFaction("Void Pirates",  FactionAlignment::Hostile);
 *   factions.SetDiplomacy(miners, pirates, DiplomaticState::War);
 *   factions.AddReputation(miners, 50, "completed_mining_quest");
 *   auto stance = factions.GetStance(miners);  // Friendly / Neutral / Hostile
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Runtime {

// ── Enums ─────────────────────────────────────────────────────────────────────

enum class FactionAlignment : uint8_t {
    Friendly,
    Neutral,
    Hostile,
    COUNT
};

enum class DiplomaticState : uint8_t {
    Neutral,
    Peace,
    Alliance,
    War,
    COUNT
};

enum class PlayerStance : uint8_t {
    Hostile,      ///< faction attacks on sight
    Cautious,     ///< faction is wary; no trade
    Neutral,      ///< faction ignores player
    Friendly,     ///< faction provides discounts
    Allied,       ///< faction assists in combat
};

// ── Data types ────────────────────────────────────────────────────────────────

struct Faction {
    uint32_t          id{0};
    std::string       name;
    FactionAlignment  defaultAlignment{FactionAlignment::Neutral};
    std::string       description;
    bool              playerJoinable{false};
};

struct ReputationEvent {
    uint32_t    factionId{0};
    float       delta{0.0f};
    std::string reason;
};

using ReputationChangedFn = std::function<void(const ReputationEvent&)>;
using DiplomacyChangedFn  = std::function<void(uint32_t factionA, uint32_t factionB,
                                                DiplomaticState newState)>;

// ── FactionSystem ─────────────────────────────────────────────────────────────

class FactionSystem {
public:
    void Init();

    // ── Factions ──────────────────────────────────────────────────────────
    uint32_t  CreateFaction(const std::string& name,
                             FactionAlignment alignment = FactionAlignment::Neutral,
                             const std::string& description = "",
                             bool playerJoinable = false);
    void      DestroyFaction(uint32_t factionId);
    Faction*  GetFaction(uint32_t factionId);
    const Faction* GetFaction(uint32_t factionId) const;
    std::vector<uint32_t> AllFactionIds() const;

    // ── Player reputation ─────────────────────────────────────────────────
    void  AddReputation(uint32_t factionId, float delta, const std::string& reason = "");
    float GetReputation(uint32_t factionId) const;
    void  SetReputation(uint32_t factionId, float value);

    PlayerStance GetStance(uint32_t factionId) const;

    static constexpr float kAlliedThreshold   =  500.0f;
    static constexpr float kFriendlyThreshold =  200.0f;
    static constexpr float kNeutralThreshold  =    0.0f;
    static constexpr float kCautiousThreshold = -200.0f;
    // below kCautiousThreshold → Hostile

    // ── Diplomacy ─────────────────────────────────────────────────────────
    void           SetDiplomacy(uint32_t factionA, uint32_t factionB,
                                 DiplomaticState state);
    DiplomaticState GetDiplomacy(uint32_t factionA, uint32_t factionB) const;

    /// Returns true if factionA is at war with factionB.
    bool IsAtWar(uint32_t factionA, uint32_t factionB) const;

    /// Returns true if two factions are allied.
    bool IsAllied(uint32_t factionA, uint32_t factionB) const;

    // ── Cross-faction reputation propagation ──────────────────────────────
    /// When the player attacks faction A, allied factions B also lose reputation.
    void PropagateHostileAct(uint32_t targetFactionId, float baseDelta);

    // ── Callbacks ─────────────────────────────────────────────────────────
    void SetReputationChangedCallback(ReputationChangedFn fn);
    void SetDiplomacyChangedCallback(DiplomacyChangedFn fn);

    // ── Stats ─────────────────────────────────────────────────────────────
    size_t FactionCount() const;
    const std::vector<ReputationEvent>& GetReputationHistory() const;
    void ClearHistory();

private:
    uint32_t DiplomacyKey(uint32_t a, uint32_t b) const;

    uint32_t m_nextId{1};
    std::unordered_map<uint32_t, Faction>         m_factions;
    std::unordered_map<uint32_t, float>            m_reputation;  ///< factionId → rep
    std::unordered_map<uint32_t, DiplomaticState>  m_diplomacy;   ///< key → state
    std::vector<ReputationEvent>                   m_history;
    ReputationChangedFn m_onRepChanged;
    DiplomacyChangedFn  m_onDipChanged;
};

} // namespace Runtime
