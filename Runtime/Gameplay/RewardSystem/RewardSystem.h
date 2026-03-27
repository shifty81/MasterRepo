#pragma once
/**
 * @file RewardSystem.h
 * @brief XP drops, loot tables with weighted chances, and reward dispatch callbacks.
 *
 * Features:
 *   - Loot tables: named tables of items with drop weights and quantity ranges
 *   - XP rules: entities award configurable XP on kill/objective/discovery
 *   - Level-up thresholds with on-level-up callbacks
 *   - Reward bundles: combine XP + loot in one dispatch
 *   - Drop source positions for world-space FX spawning
 *   - Luck multiplier per player (boosts rarity)
 *   - Rarity tiers: Common, Uncommon, Rare, Epic, Legendary with global drop-rate scalars
 *   - Conditional drops: drop only if player tag matches (e.g. faction)
 *   - JSON-driven loot table definitions
 *
 * Typical usage:
 * @code
 *   RewardSystem rs;
 *   rs.Init();
 *   rs.RegisterLootTable({"enemy_grunt", {
 *       {"credits_small", 0.8f, {5,15}, Rarity::Common},
 *       {"ammo_pack",     0.5f, {1,1},  Rarity::Uncommon},
 *       {"rare_module",   0.05f,{1,1},  Rarity::Rare}
 *   }});
 *   rs.SetOnXPGained([](uint32_t p, float xp){ UpdateHUD(p, xp); });
 *   rs.DispatchKillReward(playerId, "enemy_grunt", deathPos);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class Rarity : uint8_t { Common=0, Uncommon, Rare, Epic, Legendary };

struct LootEntry {
    std::string  itemId;
    float        dropChance{1.f};       ///< [0,1] weight (normalised internally)
    uint32_t     quantityMin{1};
    uint32_t     quantityMax{1};
    Rarity       rarity{Rarity::Common};
    std::string  requiredTag;           ///< player must have this tag
    float        luckMultiplier{1.f};   ///< per-entry luck scale
};

struct LootTableDesc {
    std::string              id;
    std::vector<LootEntry>   entries;
    uint32_t                 rollCount{1};   ///< how many entries to draw
    bool                     noDuplicates{true};
};

struct DroppedItem {
    std::string  itemId;
    uint32_t     quantity{1};
    Rarity       rarity{Rarity::Common};
};

struct RewardBundle {
    uint32_t              playerId{0};
    float                 xpAmount{0.f};
    std::string           lootTableId;
    float                 position[3]{};
    std::string           playerTag;
    float                 luckBonus{0.f};  ///< added to player's luck
};

struct RewardResult {
    uint32_t              playerId{0};
    float                 xpGained{0.f};
    std::vector<DroppedItem> items;
    float                 position[3]{};
};

struct XPRule {
    std::string sourceType;    ///< e.g. "enemy_grunt"
    float       xpAmount{50.f};
};

struct LevelThreshold {
    uint32_t level{1};
    float    totalXPRequired{0.f};
};

struct PlayerXPData {
    uint32_t playerId{0};
    float    totalXP{0.f};
    uint32_t level{1};
    float    luck{1.f};
};

class RewardSystem {
public:
    RewardSystem();
    ~RewardSystem();

    void Init();
    void Shutdown();

    // Loot tables
    void RegisterLootTable(const LootTableDesc& desc);
    void UnregisterLootTable(const std::string& id);
    bool HasLootTable(const std::string& id) const;

    // XP rules
    void RegisterXPRule(const XPRule& rule);
    void SetLevelThresholds(const std::vector<LevelThreshold>& thresholds);

    // Player data
    void     RegisterPlayer(uint32_t playerId, float initialLuck=1.f);
    void     UnregisterPlayer(uint32_t playerId);
    void     AddXP(uint32_t playerId, float xp);
    void     SetLuck(uint32_t playerId, float luck);
    const    PlayerXPData* GetPlayerData(uint32_t playerId) const;

    // Rarity drop-rate scalars
    void SetRarityScale(Rarity rarity, float scale);

    // Dispatch
    RewardResult DispatchKillReward(uint32_t playerId, const std::string& sourceType,
                                     const float position[3]);
    RewardResult DispatchBundle(const RewardBundle& bundle);
    std::vector<DroppedItem> RollLootTable(const std::string& tableId,
                                            float luck=1.f,
                                            const std::string& playerTag="");

    // Callbacks
    using XPCb      = std::function<void(uint32_t playerId, float xp)>;
    using LevelUpCb = std::function<void(uint32_t playerId, uint32_t newLevel)>;
    using ItemCb    = std::function<void(uint32_t playerId, const DroppedItem&, const float pos[3])>;
    void SetOnXPGained(XPCb cb);
    void SetOnLevelUp(LevelUpCb cb);
    void SetOnItemDropped(ItemCb cb);

    // JSON
    bool LoadFromJSON(const std::string& path);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
