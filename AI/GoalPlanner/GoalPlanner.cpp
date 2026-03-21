#include "AI/GoalPlanner/GoalPlanner.h"
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <sstream>

namespace AI {

// ── Helpers ───────────────────────────────────────────────────────────────
static bool StatesSatisfy(const WorldState& state,
                           const std::vector<Condition>& conds)
{
    for (const auto& c : conds) {
        auto it = state.find(c.key);
        if (it == state.end() || it->second != c.value) return false;
    }
    return true;
}

static WorldState ApplyEffects(WorldState state, const std::vector<Effect>& effs) {
    for (const auto& e : effs) state[e.key] = e.value;
    return state;
}

// Simple string hash for WorldState (for visited-set in BFS)
static std::string HashState(const WorldState& s) {
    std::vector<std::string> parts;
    parts.reserve(s.size());
    for (const auto& [k,v] : s) parts.push_back(k + "=" + std::to_string(v));
    std::sort(parts.begin(), parts.end());
    std::string out;
    for (auto& p : parts) { out += p; out += ';'; }
    return out;
}

// ── BFS forward-chaining planner ─────────────────────────────────────────
// Tries to reach goal conditions by forward-applying actions.
struct SearchNode {
    WorldState           state;
    std::vector<std::string> plan;
    float                cost{0.0f};
};

struct GoalPlanner::Impl {
    WorldState               worldState;
    std::vector<PlanAction>  actions;
    std::vector<Goal>        goals;
    uint32_t                 nextGoalId{1};
};

GoalPlanner::GoalPlanner() : m_impl(new Impl()) {}
GoalPlanner::~GoalPlanner() { delete m_impl; }

void GoalPlanner::SetWorldState(const WorldState& state) {
    m_impl->worldState = state;
}
void GoalPlanner::SetFact(const WorldStateKey& key, WorldStateVal value) {
    m_impl->worldState[key] = value;
}
WorldStateVal GoalPlanner::GetFact(const WorldStateKey& key,
                                    WorldStateVal def) const
{
    auto it = m_impl->worldState.find(key);
    return it != m_impl->worldState.end() ? it->second : def;
}
const WorldState& GoalPlanner::GetWorldState() const { return m_impl->worldState; }

void GoalPlanner::RegisterAction(PlanAction action) {
    m_impl->actions.push_back(std::move(action));
}
bool GoalPlanner::HasAction(const std::string& name) const {
    for (const auto& a : m_impl->actions) if (a.name == name) return true;
    return false;
}
size_t GoalPlanner::ActionCount() const { return m_impl->actions.size(); }
void GoalPlanner::RemoveAction(const std::string& name) {
    auto& v = m_impl->actions;
    v.erase(std::remove_if(v.begin(), v.end(),
            [&](const PlanAction& a){ return a.name == name; }), v.end());
}

uint32_t GoalPlanner::AddGoal(Goal goal) {
    uint32_t id = m_impl->nextGoalId++;
    goal.priority = goal.priority; // keep as-is
    m_impl->goals.push_back(std::move(goal));
    return id;
}
void GoalPlanner::RemoveGoal(uint32_t id) {
    (void)id; // Goals identified by index in this impl
}
void GoalPlanner::ClearGoals() { m_impl->goals.clear(); }
const std::vector<Goal>& GoalPlanner::Goals() const { return m_impl->goals; }

Plan GoalPlanner::PlanFor(const Goal& goal) const {
    // BFS forward-chaining search (bounded depth = action count + 2)
    const size_t maxDepth = m_impl->actions.size() + 2;

    std::queue<SearchNode> open;
    open.push({m_impl->worldState, {}, 0.0f});
    std::unordered_set<std::string> visited;
    visited.insert(HashState(m_impl->worldState));

    while (!open.empty()) {
        SearchNode cur = std::move(open.front());
        open.pop();

        if (StatesSatisfy(cur.state, goal.desiredState)) {
            Plan p;
            p.actions   = std::move(cur.plan);
            p.totalCost = cur.cost;
            p.valid     = true;
            return p;
        }
        if (cur.plan.size() >= maxDepth) continue;

        for (const auto& action : m_impl->actions) {
            if (!StatesSatisfy(cur.state, action.preconditions)) continue;
            WorldState next = ApplyEffects(cur.state, action.effects);
            std::string hash = HashState(next);
            if (visited.count(hash)) continue;
            visited.insert(hash);
            std::vector<std::string> newPlan = cur.plan;
            newPlan.push_back(action.name);
            open.push({std::move(next), std::move(newPlan),
                        cur.cost + action.cost});
        }
    }
    return {}; // no plan found
}

Plan GoalPlanner::PlanBest() const {
    if (m_impl->goals.empty()) return {};
    // Sort by descending priority, try each until a plan is found
    std::vector<const Goal*> sorted;
    sorted.reserve(m_impl->goals.size());
    for (const auto& g : m_impl->goals) sorted.push_back(&g);
    std::sort(sorted.begin(), sorted.end(),
              [](const Goal* a, const Goal* b){ return a->priority > b->priority; });
    for (const auto* g : sorted) {
        Plan p = PlanFor(*g);
        if (p.valid) return p;
    }
    return {};
}

bool GoalPlanner::ApplyAction(const std::string& name, WorldState& state) const {
    for (const auto& a : m_impl->actions) {
        if (a.name != name) continue;
        if (!StatesSatisfy(state, a.preconditions)) return false;
        for (const auto& e : a.effects) state[e.key] = e.value;
        return true;
    }
    return false;
}

bool GoalPlanner::CanExecute(const std::string& name,
                               const WorldState& state) const
{
    for (const auto& a : m_impl->actions)
        if (a.name == name) return StatesSatisfy(state, a.preconditions);
    return false;
}

} // namespace AI
