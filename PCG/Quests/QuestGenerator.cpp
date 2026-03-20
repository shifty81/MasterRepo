#include "PCG/Quests/QuestGenerator.h"
#include <algorithm>
#include <cmath>

namespace PCG {

// ──────────────────────────────────────────────────────────────
// Template registry
// ──────────────────────────────────────────────────────────────

uint32_t QuestGenerator::RegisterTemplate(QuestTemplate tmpl) {
    tmpl.id = m_nextTmplID++;
    m_templates.push_back(std::move(tmpl));
    return m_templates.back().id;
}

QuestTemplate* QuestGenerator::GetTemplate(uint32_t id) {
    for (auto& t : m_templates)
        if (t.id == id) return &t;
    return nullptr;
}

std::vector<const QuestTemplate*> QuestGenerator::GetTemplatesForLevel(
        int32_t level, const std::string& factionID) const {
    std::vector<const QuestTemplate*> out;
    for (const auto& t : m_templates) {
        if (level < t.minLevel || level > t.maxLevel) continue;
        if (!t.factionID.empty() && !factionID.empty() && t.factionID != factionID)
            continue;
        out.push_back(&t);
    }
    return out;
}

// ──────────────────────────────────────────────────────────────
// Private helpers
// ──────────────────────────────────────────────────────────────

int32_t QuestGenerator::MaxLevelForSecurity(float security) {
    if (security >= 0.8f) return 1;
    if (security >= 0.6f) return 2;
    if (security >= 0.4f) return 3;
    if (security >= 0.2f) return 4;
    return 5;
}

uint32_t QuestGenerator::LCGNext(uint32_t& rng) {
    rng = rng * 1664525u + 1013904223u;
    return rng;
}

std::vector<QuestObjective> QuestGenerator::BuildObjectives(
        const QuestTemplate& tmpl, int32_t level, uint32_t& rng) const {
    std::vector<QuestObjective> objs;
    QuestObjective obj;

    // Pick a random target if available
    std::string target;
    if (!tmpl.possibleTargets.empty()) {
        size_t idx = LCGNext(rng) % tmpl.possibleTargets.size();
        target = tmpl.possibleTargets[idx];
    }

    switch (tmpl.type) {
    case QuestType::Combat:
        obj.type         = QuestObjectiveType::Kill;
        obj.targetID     = target;
        obj.requiredCount = 1 + level * 2;
        obj.description  = "Destroy " + std::to_string(obj.requiredCount)
                         + " " + (target.empty() ? "enemies" : target);
        objs.push_back(obj);
        break;

    case QuestType::Courier:
        obj.type          = QuestObjectiveType::Deliver;
        obj.targetID      = target.empty() ? "cargo" : target;
        obj.requiredCount = 1;
        obj.description   = "Deliver " + obj.targetID + " to the destination";
        objs.push_back(obj);
        break;

    case QuestType::Mining:
        obj.type          = QuestObjectiveType::Gather;
        obj.targetID      = target.empty() ? "ore" : target;
        obj.requiredCount = 10 + level * 5;
        obj.description   = "Mine " + std::to_string(obj.requiredCount)
                          + " units of " + obj.targetID;
        objs.push_back(obj);
        break;

    case QuestType::Exploration: {
        QuestObjective reach;
        reach.type          = QuestObjectiveType::Reach;
        reach.targetID      = target.empty() ? "anomaly" : target;
        reach.requiredCount = 1;
        reach.description   = "Locate the " + reach.targetID;
        objs.push_back(reach);

        QuestObjective scan;
        scan.type          = QuestObjectiveType::Interact;
        scan.targetID      = reach.targetID;
        scan.requiredCount = 1;
        scan.description   = "Scan the " + scan.targetID;
        objs.push_back(scan);
        break;
    }

    case QuestType::Trade:
        obj.type          = QuestObjectiveType::Deliver;
        obj.targetID      = target.empty() ? "goods" : target;
        obj.requiredCount = 5 + level * 2;
        obj.description   = "Trade " + std::to_string(obj.requiredCount)
                          + " units of " + obj.targetID;
        objs.push_back(obj);
        break;

    case QuestType::Crafting:
        obj.type          = QuestObjectiveType::Craft;
        obj.targetID      = target.empty() ? "item" : target;
        obj.requiredCount = 1 + (level / 2);
        obj.description   = "Craft " + std::to_string(obj.requiredCount)
                          + " " + obj.targetID;
        objs.push_back(obj);
        break;
    }

    return objs;
}

QuestReward QuestGenerator::ScaleReward(const QuestTemplate& tmpl, int32_t level) {
    QuestReward r;
    r.currency   = tmpl.baseCurrency * level;
    r.experience = tmpl.baseXP       * level;
    return r;
}

// ──────────────────────────────────────────────────────────────
// Generation
// ──────────────────────────────────────────────────────────────

Quest QuestGenerator::GenerateFromTemplate(uint32_t templateID,
                                            const std::string& locationID,
                                            int32_t overrideLevel,
                                            uint32_t seed) {
    Quest q;
    QuestTemplate* tmpl = GetTemplate(templateID);
    if (!tmpl) return q;

    uint32_t rng = seed ^ (templateID * 2654435761u);
    int32_t level = (overrideLevel > 0) ? overrideLevel
                  : (int32_t)(tmpl->minLevel + LCGNext(rng) % (size_t)(tmpl->maxLevel - tmpl->minLevel + 1));

    q.id         = m_nextQuestID++;
    q.type       = tmpl->type;
    q.level      = level;
    q.factionID  = tmpl->factionID;
    q.locationID = locationID;
    q.seed       = seed;
    q.objectives = BuildObjectives(*tmpl, level, rng);
    q.reward     = ScaleReward(*tmpl, level);

    // Build title
    const char* typeNames[] = {"Combat","Courier","Mining","Exploration","Trade","Crafting"};
    q.title = std::string(typeNames[(int)tmpl->type]) + " Mission [L" + std::to_string(level) + "]";
    q.description = tmpl->name + " — " + locationID;

    return q;
}

std::vector<Quest> QuestGenerator::GenerateForLocation(const LocationContext& ctx,
                                                        uint32_t seed) {
    m_available[ctx.locationID].clear();
    int32_t maxLevel = MaxLevelForSecurity(ctx.security);

    uint32_t rng = seed;
    for (const auto& tmpl : m_templates) {
        if (tmpl.maxLevel < 1 || tmpl.minLevel > maxLevel) continue;

        // Filter by location conditions
        bool ok = false;
        switch (tmpl.type) {
        case QuestType::Combat:     ok = true; break;
        case QuestType::Courier:    ok = true; break;
        case QuestType::Trade:      ok = ctx.hasShop; break;
        case QuestType::Mining:     ok = ctx.hasMinerals; break;
        case QuestType::Exploration: ok = ctx.hasAnomalies; break;
        case QuestType::Crafting:   ok = ctx.hasShop; break;
        }
        if (!ok) continue;

        int32_t level = (int32_t)(tmpl.minLevel + LCGNext(rng) % (size_t)(maxLevel - tmpl.minLevel + 1));
        level = std::min(level, maxLevel);

        Quest q = GenerateFromTemplate(tmpl.id, ctx.locationID, level,
                                       LCGNext(rng) ^ seed);
        m_available[ctx.locationID].push_back(std::move(q));
    }

    return m_available[ctx.locationID];
}

bool QuestGenerator::AcceptQuest(uint32_t locationQuestIndex,
                                  const std::string& locationID,
                                  std::vector<Quest>& playerActiveQuests) {
    auto it = m_available.find(locationID);
    if (it == m_available.end()) return false;
    auto& quests = it->second;
    if (locationQuestIndex >= quests.size()) return false;
    playerActiveQuests.push_back(quests[locationQuestIndex]);
    quests.erase(quests.begin() + (ptrdiff_t)locationQuestIndex);
    return true;
}

bool QuestGenerator::UpdateObjective(Quest& quest, QuestObjectiveType type,
                                      const std::string& targetID, int32_t delta) {
    for (auto& obj : quest.objectives) {
        if (obj.completed) continue;
        if (obj.type != type) continue;
        if (!targetID.empty() && obj.targetID != targetID) continue;
        obj.currentCount += delta;
        if (obj.currentCount >= obj.requiredCount) {
            obj.currentCount = obj.requiredCount;
            obj.completed    = true;
        }
    }
    // Quest complete when all objectives done
    for (const auto& obj : quest.objectives)
        if (!obj.completed) return false;
    return true;
}

const std::vector<Quest>& QuestGenerator::AvailableAt(const std::string& locationID) const {
    static const std::vector<Quest> empty;
    auto it = m_available.find(locationID);
    return (it != m_available.end()) ? it->second : empty;
}

} // namespace PCG
