#pragma once
/**
 * @file LootTable.h
 * @brief Weighted random loot roll: item pools, rarity tiers, guaranteed drops, seed, multi-roll.
 *
 * Features:
 *   - LootEntry: itemId, weight, minQty, maxQty, rarityTier
 *   - LootPool: list of entries, rollCount (rolls per invocation), guaranteed list
 *   - RegisterPool(poolId, pool)
 *   - Roll(poolId, seed) → vector of LootResult {itemId, qty}
 *   - MultiRoll(poolId, n, seed) → vector of LootResult (combined)
 *   - SetGlobalLuckMultiplier(f): scales weights of higher-tier entries
 *   - DropOverride: force-inject extra entries for an event
 *   - GetPoolEntries(poolId)
 *   - SimulateDropRates(poolId, n) → per-entry drop %, for debug
 *   - Seeded: reproducible results for replays
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Runtime {

enum class RarityTier : uint8_t { Common=0, Uncommon, Rare, Epic, Legendary };

struct LootEntry {
    std::string id;
    float       weight   {1.f};
    uint32_t    minQty   {1};
    uint32_t    maxQty   {1};
    RarityTier  rarity   {RarityTier::Common};
};

struct LootResult {
    std::string itemId;
    uint32_t    qty{1};
    RarityTier  rarity{RarityTier::Common};
};

struct LootPool {
    std::vector<LootEntry>  entries;
    uint32_t                rollCount  {1};
    std::vector<std::string> guaranteed; ///< always included (itemIds)
};

class LootTable {
public:
    LootTable();
    ~LootTable();

    void Init    ();
    void Shutdown();

    // Pool management
    void          RegisterPool  (const std::string& poolId, const LootPool& pool);
    void          UnregisterPool(const std::string& poolId);
    const LootPool* GetPool     (const std::string& poolId) const;

    // Rolling
    std::vector<LootResult> Roll     (const std::string& poolId, uint32_t seed=0) const;
    std::vector<LootResult> MultiRoll(const std::string& poolId, uint32_t n, uint32_t seed=0) const;

    // Luck
    void SetGlobalLuckMultiplier(float f); ///< boosts higher-tier weight

    // Drop overrides (temporary injection)
    void AddDropOverride   (const std::string& poolId, const LootEntry& entry);
    void ClearDropOverrides(const std::string& poolId);

    // Debug
    std::vector<std::pair<std::string,float>> SimulateDropRates(const std::string& poolId,
                                                                  uint32_t n=10000) const;
    std::vector<std::string> GetAllPoolIds() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
