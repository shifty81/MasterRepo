#pragma once
/**
 * @file ProgressionSystem.h
 * @brief Player progression — XP, levels, tech unlocks, and talent points.
 *
 * Feeds into Runtime::SkillTree for per-skill effects.
 * Manages:
 *   - XP sources (combat kills, crafting, exploration, quests)
 *   - Level thresholds and level-up events
 *   - Talent points awarded on level-up
 *   - Tech tree integration: unlock technologies via talent points
 *   - Soft-cap beyond max level (prestige levels)
 *
 * Usage:
 * @code
 *   ProgressionSystem progression;
 *   progression.Init();
 *   progression.AddXP(XPSource::Combat, 150.0f);
 *   if (progression.GetLevel() > 1)
 *       progression.SpendTalentPoint("hull_reinforcement");
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Runtime {

// ── XP source ─────────────────────────────────────────────────────────────────

enum class XPSource : uint8_t {
    Combat,
    Crafting,
    Exploration,
    Quest,
    Trade,
    Building,
    Other,
    COUNT
};

// ── Tech node ─────────────────────────────────────────────────────────────────

struct TechNode {
    uint32_t               id{0};
    std::string            name;
    std::string            description;
    uint32_t               talentPointCost{1};
    std::vector<uint32_t>  prerequisites;  ///< tech node IDs
    bool                   unlocked{false};
};

// ── Level-up event ────────────────────────────────────────────────────────────

struct LevelUpEvent {
    uint32_t newLevel{0};
    uint32_t talentPointsGranted{0};
    float    totalXP{0.0f};
};

using LevelUpFn        = std::function<void(const LevelUpEvent&)>;
using TechUnlockedFn   = std::function<void(uint32_t techNodeId, const std::string& name)>;

// ── XP record ─────────────────────────────────────────────────────────────────

struct XPRecord {
    XPSource    source{XPSource::Other};
    float       amount{0.0f};
    std::string label;
};

// ── ProgressionSystem ─────────────────────────────────────────────────────────

class ProgressionSystem {
public:
    void Init(uint32_t maxLevel = 50);

    // ── XP & Levels ───────────────────────────────────────────────────────
    void    AddXP(XPSource source, float amount, const std::string& label = "");
    float   GetXP() const;
    uint32_t GetLevel() const;
    uint32_t GetMaxLevel() const;
    float   GetXPForNextLevel() const;  ///< XP needed to reach the next level
    float   GetLevelProgress() const;   ///< 0..1 progress towards next level

    // ── Talent points ─────────────────────────────────────────────────────
    uint32_t GetTalentPoints()      const;
    bool     SpendTalentPoint(uint32_t techNodeId);
    bool     SpendTalentPoint(const std::string& techName);

    // ── Tech tree ─────────────────────────────────────────────────────────
    uint32_t AddTechNode(const std::string& name,
                          const std::string& description,
                          uint32_t cost = 1,
                          const std::vector<uint32_t>& prerequisites = {});
    bool     IsTechUnlocked(uint32_t techNodeId) const;
    bool     CanUnlockTech(uint32_t techNodeId)  const;
    const TechNode* GetTechNode(uint32_t id) const;
    std::vector<uint32_t> AllTechIds() const;

    // ── Prestige ──────────────────────────────────────────────────────────
    bool     CanPrestige() const;  ///< true if at max level
    void     Prestige();           ///< reset level, keep tech, gain prestige bonus

    uint32_t PrestigeLevel() const;

    // ── XP multipliers per source ─────────────────────────────────────────
    void  SetXPMultiplier(XPSource source, float multiplier);
    float GetXPMultiplier(XPSource source) const;

    // ── Callbacks ─────────────────────────────────────────────────────────
    void SetLevelUpCallback(LevelUpFn fn);
    void SetTechUnlockedCallback(TechUnlockedFn fn);

    // ── Stats ─────────────────────────────────────────────────────────────
    const std::vector<XPRecord>& GetXPHistory() const;
    void ClearXPHistory();

private:
    float XPForLevel(uint32_t level) const;  ///< total XP to reach that level
    void CheckLevelUp();

    uint32_t m_maxLevel{50};
    uint32_t m_level{1};
    uint32_t m_prestigeLevel{0};
    float    m_totalXP{0.0f};
    uint32_t m_talentPoints{0};
    uint32_t m_nextTechId{1};

    std::unordered_map<uint32_t, TechNode>   m_tech;
    std::unordered_map<XPSource, float>      m_xpMult;
    std::vector<XPRecord>                    m_xpHistory;

    LevelUpFn      m_onLevelUp;
    TechUnlockedFn m_onTechUnlocked;
};

} // namespace Runtime
