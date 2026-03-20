#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace PCG {

// ──────────────────────────────────────────────────────────────
// Quest data model
// ──────────────────────────────────────────────────────────────

enum class QuestType { Combat, Courier, Mining, Exploration, Trade, Crafting };

enum class QuestObjectiveType { Kill, Deliver, Gather, Reach, Interact, Craft };

struct QuestObjective {
    QuestObjectiveType type       = QuestObjectiveType::Kill;
    std::string        targetID;      // entity/item type id
    std::string        description;
    int32_t            requiredCount = 1;
    int32_t            currentCount  = 0;
    bool               completed     = false;
};

struct QuestReward {
    int32_t     currency     = 0;
    int32_t     experience   = 0;
    std::vector<std::string> itemIDs;
};

struct Quest {
    uint32_t                    id          = 0;
    std::string                 title;
    std::string                 description;
    QuestType                   type        = QuestType::Combat;
    int32_t                     level       = 1;
    std::string                 factionID;
    std::string                 locationID;
    std::vector<QuestObjective> objectives;
    QuestReward                 reward;
    bool                        repeatable  = false;
    uint32_t                    seed        = 0;
};

// ──────────────────────────────────────────────────────────────
// Template — defines the shape of quests of a given type
// ──────────────────────────────────────────────────────────────

struct QuestTemplate {
    uint32_t    id          = 0;
    std::string name;
    QuestType   type        = QuestType::Combat;
    int32_t     minLevel    = 1;
    int32_t     maxLevel    = 5;
    std::string factionID;                         // empty = any faction
    std::vector<std::string> possibleTargets;      // entity/item types
    int32_t     baseCurrency  = 100;
    int32_t     baseXP        = 50;
};

// ──────────────────────────────────────────────────────────────
// QuestGenerator — migrated from atlas::systems::MissionGeneratorSystem
// ──────────────────────────────────────────────────────────────

class QuestGenerator {
public:
    // Template registry
    uint32_t          RegisterTemplate(QuestTemplate tmpl);
    QuestTemplate*    GetTemplate(uint32_t id);
    std::vector<const QuestTemplate*> GetTemplatesForLevel(int32_t level,
                                                           const std::string& factionID = "") const;

    // Environment context for the generator (mirrors DifficultyZone/SystemResources)
    struct LocationContext {
        std::string locationID;
        float       security        = 0.5f;   // 0=dangerous → 1=safe
        bool        hasMinerals     = false;
        bool        hasAnomalies    = false;
        bool        hasShop         = true;
    };

    // Generate all quests available at a location (deterministic with seed)
    std::vector<Quest> GenerateForLocation(const LocationContext& ctx, uint32_t seed);

    // Generate a single quest from a specific template
    Quest GenerateFromTemplate(uint32_t templateID,
                               const std::string& locationID,
                               int32_t overrideLevel = 0,
                               uint32_t seed = 0);

    // Offer a quest to a player (marks it as taken, removes from available pool)
    bool AcceptQuest(uint32_t locationQuestIndex,
                     const std::string& locationID,
                     std::vector<Quest>& playerActiveQuests);

    // Update objective progress; returns true when quest is completed
    static bool UpdateObjective(Quest& quest, QuestObjectiveType type,
                                const std::string& targetID, int32_t delta = 1);

    // Pending quests for a location (populated by GenerateForLocation)
    const std::vector<Quest>& AvailableAt(const std::string& locationID) const;

private:
    uint32_t                               m_nextTmplID = 1;
    uint32_t                               m_nextQuestID = 1;
    std::vector<QuestTemplate>             m_templates;
    std::map<std::string, std::vector<Quest>> m_available; // locationID → quests

    // Determine max quest level for a security rating
    static int32_t MaxLevelForSecurity(float security);
    // Simple LCG for deterministic picks
    static uint32_t LCGNext(uint32_t& rng);
    // Build objectives for a quest based on its template
    std::vector<QuestObjective> BuildObjectives(const QuestTemplate& tmpl,
                                                int32_t level, uint32_t& rng) const;
    // Scale reward with level
    static QuestReward ScaleReward(const QuestTemplate& tmpl, int32_t level);
};

} // namespace PCG
