#pragma once
/**
 * @file ComboSystem.h
 * @brief Attack combo chain system with timing windows, branching, and finishers.
 *
 * Features:
 *   - Combo trees: each node defines an attack move and an input to continue
 *   - Configurable link window (early, perfect, late) with bonus multipliers
 *   - Branching combos (different follow-ups from the same node)
 *   - Combo break on miss / wrong input
 *   - Finisher moves (special move at end of chain)
 *   - Multiple concurrent combo states (split-screen / multiple fighters)
 *   - Combo counter and longest-combo tracking per fighter
 *   - JSON definition loading
 *
 * Typical usage:
 * @code
 *   ComboSystem cs;
 *   cs.Init();
 *   cs.LoadDefinition("Data/Combos/warrior.json");
 *   uint32_t fighter = cs.RegisterFighter(fighterId);
 *   // per-frame
 *   cs.FeedInput(fighter, "light");
 *   cs.Tick(dt);
 *   if (cs.HasPendingAttack(fighter)) {
 *       auto atk = cs.ConsumeAttack(fighter);
 *       ApplyAttack(atk);
 *   }
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct ComboLinkWindow {
    float earlyThresh{0.1f};    ///< s before perfect window — partial bonus
    float perfectStart{0.2f};   ///< s — perfect window start (relative to move start)
    float perfectEnd{0.35f};    ///< s — perfect window end
    float lateThresh{0.5f};     ///< s — latest acceptable input
    float earlyBonus{1.0f};
    float perfectBonus{1.5f};
    float lateBonus{0.8f};
};

struct ComboNode {
    uint32_t          nodeId{0};
    std::string       moveName;         ///< e.g. "slash_01"
    std::string       inputRequired;    ///< e.g. "light"
    float             duration{0.4f};   ///< seconds this move lasts
    ComboLinkWindow   linkWindow{};
    bool              isFinisher{false};
    float             damageMultiplier{1.f};
    std::vector<uint32_t> followUps;    ///< child node IDs
};

struct ComboDefinition {
    std::string              id;
    std::vector<ComboNode>   nodes;
    std::vector<uint32_t>    rootNodes;  ///< nodes reachable from idle
};

enum class ComboLinkQuality : uint8_t { None=0, Early, Perfect, Late };

struct AttackResult {
    std::string       moveName;
    float             damageMultiplier{1.f};
    ComboLinkQuality  linkQuality{ComboLinkQuality::Perfect};
    bool              isFinisher{false};
    uint32_t          comboCount{0};    ///< current chain length
};

struct FighterStats {
    uint32_t  currentComboLength{0};
    uint32_t  longestCombo{0};
    uint32_t  totalHits{0};
    float     timeSinceLastHit{0.f};
};

class ComboSystem {
public:
    ComboSystem();
    ~ComboSystem();

    void Init();
    void Shutdown();
    void Tick(float dt);

    // Definitions
    void LoadDefinition(const ComboDefinition& def);
    bool LoadFromJSON(const std::string& path);

    // Fighter management
    uint32_t RegisterFighter(uint32_t entityId, const std::string& comboDefId="");
    void     UnregisterFighter(uint32_t handle);
    void     SetComboDefinition(uint32_t handle, const std::string& comboDefId);

    // Input
    void     FeedInput(uint32_t handle, const std::string& inputName);
    void     Break(uint32_t handle);           ///< force reset chain
    void     OnMoveComplete(uint32_t handle);  ///< notify system move animation ended

    // Query
    bool         HasPendingAttack(uint32_t handle) const;
    AttackResult ConsumeAttack(uint32_t handle);
    FighterStats GetStats(uint32_t handle) const;
    bool         IsIdle(uint32_t handle) const;

    // Events
    using ComboBreakCb  = std::function<void(uint32_t handle, uint32_t length)>;
    using FinisherCb    = std::function<void(uint32_t handle, const AttackResult&)>;
    void SetOnComboBreak(ComboBreakCb cb);
    void SetOnFinisher(FinisherCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
