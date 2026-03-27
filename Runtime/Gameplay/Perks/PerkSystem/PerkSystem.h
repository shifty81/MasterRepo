#pragma once
/**
 * @file PerkSystem.h
 * @brief Perk registry, tier tree, unlock/lock, prerequisites, stack count, callbacks.
 *
 * Features:
 *   - PerkDef: id, name, tier, maxStacks, prerequisiteIds, description
 *   - RegisterPerk(def) → perkId
 *   - UnlockPerk(playerId, perkId) — checks prerequisites, increments stacks
 *   - LockPerk(playerId, perkId) — removes one stack or all
 *   - IsUnlocked(playerId, perkId) → bool
 *   - GetStackCount(playerId, perkId) → uint32_t
 *   - GetAllPerks(playerId) → list of (perkId, stacks) pairs
 *   - GetAvailablePerks(playerId) → unlockable but not yet unlocked
 *   - On-unlock callback per perk
 *   - Per-player perk state (multiple players)
 *   - SaveState(playerId) / LoadState(playerId) → JSON-compatible string
 *
 * Typical usage:
 * @code
 *   PerkSystem ps;
 *   ps.Init();
 *   PerkDef d; d.id="speed_boost"; d.name="Speed Boost"; d.tier=1; d.maxStacks=3;
 *   ps.RegisterPerk(d);
 *   ps.UnlockPerk(0, "speed_boost");
 *   int s = ps.GetStackCount(0, "speed_boost"); // 1
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct PerkDef {
    std::string id;
    std::string name;
    std::string description;
    uint32_t    tier     {1};
    uint32_t    maxStacks{1};
    std::vector<std::string> prerequisiteIds;
};

struct PerkState {
    std::string perkId;
    uint32_t    stacks{0};
};

class PerkSystem {
public:
    PerkSystem();
    ~PerkSystem();

    void Init    ();
    void Shutdown();

    // Registry
    void           RegisterPerk   (const PerkDef& def);
    void           UnregisterPerk (const std::string& id);
    const PerkDef* GetPerkDef     (const std::string& id) const;
    std::vector<std::string> GetAllPerkIds() const;

    // Player perks
    bool     UnlockPerk    (uint32_t playerId, const std::string& perkId);
    void     LockPerk      (uint32_t playerId, const std::string& perkId, bool allStacks=false);
    bool     IsUnlocked    (uint32_t playerId, const std::string& perkId) const;
    uint32_t GetStackCount (uint32_t playerId, const std::string& perkId) const;
    bool     CanUnlock     (uint32_t playerId, const std::string& perkId) const;

    std::vector<PerkState>   GetPlayerPerks     (uint32_t playerId) const;
    std::vector<std::string> GetAvailablePerks  (uint32_t playerId) const;

    // Callbacks
    void SetOnUnlock(std::function<void(uint32_t playerId, const std::string& perkId)> cb);

    // Serialisation
    std::string SaveState (uint32_t playerId) const;
    void        LoadState (uint32_t playerId, const std::string& json);

    void ResetPlayer(uint32_t playerId);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
