#pragma once
/**
 * @file TalentTree.h
 * @brief Branching talent graph, points budget, node activation, respec, serialise.
 *
 * Features:
 *   - TalentNode: id, name, description, tier, maxRank, prerequisiteIds, pointCost
 *   - RegisterNode(def)
 *   - Spend(playerId, nodeId) — activates/upgrades if prerequisites met and points available
 *   - GetRank(playerId, nodeId) → uint32_t
 *   - IsActive(playerId, nodeId) → bool
 *   - GetSpentPoints(playerId) / GetTotalPoints(playerId)
 *   - AddPoints(playerId, n)
 *   - Respec(playerId): refunds all points
 *   - CanSpend(playerId, nodeId) → bool
 *   - GetAvailableNodes(playerId)
 *   - Per-player state; on-activate callback
 *   - SaveState/LoadState (JSON-compatible string)
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct TalentNode {
    std::string id;
    std::string name;
    std::string description;
    uint32_t    tier      {1};
    uint32_t    maxRank   {1};
    uint32_t    pointCost {1};
    std::vector<std::string> prerequisiteIds;
};

class TalentTree {
public:
    TalentTree();
    ~TalentTree();

    void Init    ();
    void Shutdown();

    // Tree definition
    void               RegisterNode  (const TalentNode& def);
    void               UnregisterNode(const std::string& id);
    const TalentNode*  GetNode       (const std::string& id) const;
    std::vector<std::string> GetAllNodeIds() const;

    // Player
    void AddPoints  (uint32_t playerId, uint32_t n);
    bool Spend      (uint32_t playerId, const std::string& nodeId);
    bool CanSpend   (uint32_t playerId, const std::string& nodeId) const;
    bool IsActive   (uint32_t playerId, const std::string& nodeId) const;
    uint32_t GetRank(uint32_t playerId, const std::string& nodeId) const;

    uint32_t GetSpentPoints    (uint32_t playerId) const;
    uint32_t GetRemainingPoints(uint32_t playerId) const;
    void     Respec            (uint32_t playerId);

    std::vector<std::string> GetAvailableNodes(uint32_t playerId) const;
    std::vector<std::string> GetActiveNodes   (uint32_t playerId) const;

    void SetOnActivate(std::function<void(uint32_t playerId, const std::string& nodeId, uint32_t rank)> cb);

    std::string SaveState (uint32_t playerId) const;
    void        LoadState (uint32_t playerId, const std::string& json);
    void        ResetPlayer(uint32_t playerId);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
