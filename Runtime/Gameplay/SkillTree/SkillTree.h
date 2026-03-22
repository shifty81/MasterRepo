#pragma once
/**
 * @file SkillTree.h
 * @brief Player skill tree with prerequisites, effects, and unlock validation.
 *
 * Adapted from Nova-Forge-Expeditions (atlas::gameplay → Runtime namespace).
 *
 * SkillTree manages a directed graph of skills where each node may require
 * others to be unlocked first:
 *
 *   SkillEffect: { systemName, ModifierType (Add/Multiply/Clamp), value }
 *   SkillNode: id, name, prerequisites[], effects[], unlocked flag.
 *
 *   SkillTree:
 *     - AddNode(name, prerequisites, effects) → SkillID
 *     - Unlock(id): unlocks if all prerequisites are already unlocked.
 *     - IsUnlocked(id) / CanUnlock(id)
 *     - GetEffects(id): list of effects if unlocked.
 *     - GetNode(id): read-only node pointer.
 *     - NodeCount()
 */

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Runtime {

using SkillID = uint32_t;

// ── Modifier type ─────────────────────────────────────────────────────────
enum class ModifierType : uint8_t { Add, Multiply, Clamp };

// ── Skill effect ──────────────────────────────────────────────────────────
struct SkillEffect {
    std::string  systemName;
    ModifierType modifierType{ModifierType::Add};
    float        value{0.0f};
};

// ── Skill node ────────────────────────────────────────────────────────────
struct SkillNode {
    SkillID                   id{0};
    std::string               name;
    std::vector<SkillID>      prerequisites;
    std::vector<SkillEffect>  effects;
    bool                      unlocked{false};
};

// ── Skill tree ────────────────────────────────────────────────────────────
class SkillTree {
public:
    void Init();

    SkillID AddNode(const std::string& name,
                    const std::vector<SkillID>&     prerequisites = {},
                    const std::vector<SkillEffect>& effects       = {});

    bool Unlock(SkillID id);
    bool IsUnlocked(SkillID id) const;
    bool CanUnlock(SkillID id)  const;

    std::vector<SkillEffect> GetEffects(SkillID id) const;
    const SkillNode*         GetNode(SkillID id)    const;
    size_t                   NodeCount()            const;

    /// All currently unlocked skill ids.
    std::vector<SkillID> UnlockedSkills() const;

    /// All skills available to unlock (prerequisites satisfied, not yet unlocked).
    std::vector<SkillID> AvailableToUnlock() const;

    void Reset();   ///< Lock all skills (preserve structure)

private:
    std::unordered_map<SkillID, SkillNode> m_nodes;
    SkillID m_nextId{1};
};

} // namespace Runtime
