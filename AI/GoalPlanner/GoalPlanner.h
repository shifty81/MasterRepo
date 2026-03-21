#pragma once
/**
 * @file GoalPlanner.h
 * @brief GOAP-style goal-oriented action planner for AI agents/NPCs.
 *
 * The GoalPlanner implements a lightweight Goal-Oriented Action Planning
 * algorithm:
 *   1. The world state is a flat set of named boolean/integer facts.
 *   2. Actions have preconditions (required world-state) and effects
 *      (modifications to world-state after execution).
 *   3. Given a goal (desired world-state), the planner performs a
 *      backward-chaining BFS/A* search to find the cheapest action
 *      sequence that transitions the current state to the goal.
 *
 * Designed for NPC decision-making but equally usable for agent scheduling.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

namespace AI {

// ── World state ───────────────────────────────────────────────────────────
using WorldStateKey = std::string;
using WorldStateVal = int32_t;       ///< 0/1 for booleans, any int for counts
using WorldState    = std::unordered_map<WorldStateKey, WorldStateVal>;

// ── Conditions & effects ──────────────────────────────────────────────────
struct Condition {
    WorldStateKey key;
    WorldStateVal value{1};   ///< Required value (default = true/1)
};

struct Effect {
    WorldStateKey key;
    WorldStateVal value{1};
};

// ── Action definition ─────────────────────────────────────────────────────
struct PlanAction {
    std::string            name;
    float                  cost{1.0f};
    std::vector<Condition> preconditions;
    std::vector<Effect>    effects;
};

// ── Plan ─────────────────────────────────────────────────────────────────
struct Plan {
    std::vector<std::string> actions;   ///< Ordered action names to execute
    float                    totalCost{0.0f};
    bool                     valid{false};
};

// ── Goal ─────────────────────────────────────────────────────────────────
struct Goal {
    std::string            name;
    float                  priority{1.0f};
    std::vector<Condition> desiredState;
};

/// GoalPlanner — backward-chaining GOAP planner.
class GoalPlanner {
public:
    GoalPlanner();
    ~GoalPlanner();

    // ── world state ───────────────────────────────────────────
    void SetWorldState(const WorldState& state);
    void SetFact(const WorldStateKey& key, WorldStateVal value);
    WorldStateVal GetFact(const WorldStateKey& key,
                          WorldStateVal defaultVal = 0) const;
    const WorldState& GetWorldState() const;

    // ── actions ───────────────────────────────────────────────
    void RegisterAction(PlanAction action);
    bool HasAction(const std::string& name) const;
    size_t ActionCount() const;
    void RemoveAction(const std::string& name);

    // ── goals ─────────────────────────────────────────────────
    uint32_t AddGoal(Goal goal);
    void RemoveGoal(uint32_t id);
    void ClearGoals();
    const std::vector<Goal>& Goals() const;

    // ── planning ──────────────────────────────────────────────
    /// Plan toward a specific goal using the current world state.
    Plan PlanFor(const Goal& goal) const;

    /// Select the highest-priority achievable goal and plan for it.
    Plan PlanBest() const;

    // ── execution helpers ─────────────────────────────────────
    /// Apply a named action's effects to the provided world state.
    bool ApplyAction(const std::string& name, WorldState& state) const;

    /// Check whether all preconditions of an action are satisfied.
    bool CanExecute(const std::string& name,
                    const WorldState& state) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace AI
