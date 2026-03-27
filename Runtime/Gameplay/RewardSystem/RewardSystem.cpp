#include "Runtime/Gameplay/RewardSystem/RewardSystem.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct RewardSystem::Impl {
    std::unordered_map<std::string, LootTableDesc> tables;
    std::vector<XPRule>                             xpRules;
    std::vector<LevelThreshold>                     levelThresholds;
    std::unordered_map<uint32_t, PlayerXPData>      players;
    float rarityScales[5]{1.f,1.f,1.f,1.f,1.f};

    XPCb      onXP;
    LevelUpCb onLevelUp;
    ItemCb    onItem;

    float GetXPForSource(const std::string& src) const {
        for (auto& r : xpRules) if (r.sourceType==src) return r.xpAmount;
        return 0.f;
    }

    uint32_t CalcLevel(float totalXP) const {
        uint32_t lvl=1;
        for (auto& t : levelThresholds)
            if (totalXP >= t.totalXPRequired) lvl=t.level;
        return lvl;
    }

    std::vector<DroppedItem> Roll(const LootTableDesc& tbl, float luck,
                                   const std::string& playerTag) const {
        std::vector<DroppedItem> drops;
        // Build weighted pool
        std::vector<std::pair<float,const LootEntry*>> pool;
        float totalW = 0.f;
        for (auto& e : tbl.entries) {
            if (!e.requiredTag.empty() && e.requiredTag!=playerTag) continue;
            float rs = rarityScales[(int)e.rarity];
            float w  = e.dropChance * luck * e.luckMultiplier * rs;
            pool.push_back({w, &e});
            totalW += w;
        }
        if (totalW <= 0.f) return drops;

        std::vector<uint32_t> chosen;
        uint32_t rolls = tbl.rollCount;
        for (uint32_t r=0; r<rolls; r++) {
            float rnd = totalW * (float)(std::rand()%10000)/10000.f;
            float acc = 0.f;
            for (uint32_t pi=0; pi<(uint32_t)pool.size(); pi++) {
                acc += pool[pi].first;
                if (rnd <= acc) {
                    if (tbl.noDuplicates &&
                        std::find(chosen.begin(),chosen.end(),pi)!=chosen.end())
                        break;
                    chosen.push_back(pi);
                    const LootEntry* ep = pool[pi].second;
                    uint32_t qty = ep->quantityMin;
                    if (ep->quantityMax > ep->quantityMin)
                        qty += std::rand()%(ep->quantityMax-ep->quantityMin+1);
                    DroppedItem di; di.itemId=ep->itemId; di.quantity=qty; di.rarity=ep->rarity;
                    drops.push_back(di);
                    break;
                }
            }
        }
        return drops;
    }
};

RewardSystem::RewardSystem()  : m_impl(new Impl) {}
RewardSystem::~RewardSystem() { Shutdown(); delete m_impl; }

void RewardSystem::Init()     {}
void RewardSystem::Shutdown() {}

void RewardSystem::RegisterLootTable(const LootTableDesc& desc)
{ m_impl->tables[desc.id] = desc; }

void RewardSystem::UnregisterLootTable(const std::string& id)
{ m_impl->tables.erase(id); }

bool RewardSystem::HasLootTable(const std::string& id) const
{ return m_impl->tables.count(id)>0; }

void RewardSystem::RegisterXPRule(const XPRule& rule)
{ m_impl->xpRules.push_back(rule); }

void RewardSystem::SetLevelThresholds(const std::vector<LevelThreshold>& t)
{
    m_impl->levelThresholds = t;
    std::sort(m_impl->levelThresholds.begin(), m_impl->levelThresholds.end(),
              [](auto& a,auto& b){ return a.totalXPRequired < b.totalXPRequired; });
}

void RewardSystem::RegisterPlayer(uint32_t playerId, float initialLuck)
{
    PlayerXPData d; d.playerId=playerId; d.luck=initialLuck;
    m_impl->players[playerId] = d;
}

void RewardSystem::UnregisterPlayer(uint32_t playerId)
{ m_impl->players.erase(playerId); }

void RewardSystem::AddXP(uint32_t playerId, float xp)
{
    auto it = m_impl->players.find(playerId);
    if (it == m_impl->players.end()) return;
    auto& pd = it->second;
    pd.totalXP += xp;
    if (m_impl->onXP) m_impl->onXP(playerId, xp);

    uint32_t newLevel = m_impl->CalcLevel(pd.totalXP);
    if (newLevel > pd.level) {
        pd.level = newLevel;
        if (m_impl->onLevelUp) m_impl->onLevelUp(playerId, newLevel);
    }
}

void RewardSystem::SetLuck(uint32_t playerId, float luck)
{
    auto it = m_impl->players.find(playerId);
    if (it != m_impl->players.end()) it->second.luck = luck;
}

const PlayerXPData* RewardSystem::GetPlayerData(uint32_t playerId) const
{
    auto it = m_impl->players.find(playerId);
    return it != m_impl->players.end() ? &it->second : nullptr;
}

void RewardSystem::SetRarityScale(Rarity rarity, float scale)
{
    m_impl->rarityScales[(int)rarity] = scale;
}

RewardResult RewardSystem::DispatchKillReward(uint32_t playerId,
                                               const std::string& sourceType,
                                               const float position[3])
{
    RewardBundle b;
    b.playerId  = playerId;
    b.xpAmount  = m_impl->GetXPForSource(sourceType);
    b.lootTableId = sourceType;
    if (position) for(int i=0;i<3;i++) b.position[i]=position[i];
    auto it = m_impl->players.find(playerId);
    if (it != m_impl->players.end()) b.luckBonus = it->second.luck - 1.f;
    return DispatchBundle(b);
}

RewardResult RewardSystem::DispatchBundle(const RewardBundle& bundle)
{
    RewardResult r;
    r.playerId = bundle.playerId;
    for(int i=0;i<3;i++) r.position[i]=bundle.position[i];

    // XP
    if (bundle.xpAmount > 0.f) { AddXP(bundle.playerId, bundle.xpAmount); r.xpGained=bundle.xpAmount; }

    // Loot
    r.items = RollLootTable(bundle.lootTableId, 1.f + bundle.luckBonus, bundle.playerTag);

    if (m_impl->onItem) {
        for (auto& di : r.items) m_impl->onItem(bundle.playerId, di, bundle.position);
    }
    return r;
}

std::vector<DroppedItem> RewardSystem::RollLootTable(const std::string& tableId,
                                                       float luck,
                                                       const std::string& playerTag)
{
    auto it = m_impl->tables.find(tableId);
    if (it == m_impl->tables.end()) return {};
    return m_impl->Roll(it->second, luck, playerTag);
}

void RewardSystem::SetOnXPGained(XPCb cb)      { m_impl->onXP      = cb; }
void RewardSystem::SetOnLevelUp(LevelUpCb cb)   { m_impl->onLevelUp = cb; }
void RewardSystem::SetOnItemDropped(ItemCb cb)  { m_impl->onItem    = cb; }

bool RewardSystem::LoadFromJSON(const std::string& path)
{
    std::ifstream f(path);
    return f.good();
}

} // namespace Runtime
