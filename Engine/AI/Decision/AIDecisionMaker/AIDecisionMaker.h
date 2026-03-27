#pragma once
/**
 * @file AIDecisionMaker.h
 * @brief Utility-theory AI decision system: action scoring, goal registry, blackboard, and plan.
 *
 * Features:
 *   - RegisterGoal(name, priority): add a goal with a priority weight
 *   - RegisterAction(name, goalName, scorer): action with a scoring function
 *   - Blackboard: SetValue(key, float) / GetValue(key): shared world-state data
 *   - SelectBestAction(agentId) → actionName: runs all scorers and picks the highest
 *   - ExecuteAction(agentId, actionName): fire OnExecute callback
 *   - SetOnExecute(actionName, cb): bind execution callback
 *   - Plan(agentId, depth) → actionName[]: greedy forward-plan of N steps
 *   - SetScorerContext(agentId, contextData*): agent-specific context passed to scorers
 *   - SetCooldown(actionName, seconds): prevent re-selection within cooldown window
 *   - Tick(dt): advance cooldown timers
 *   - GetLastAction(agentId) → actionName
 *   - DebugDump() → string: human-readable score table for last evaluation
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Engine {

struct AIBlackboard {
    std::unordered_map<std::string,float> values;
    void  Set(const std::string& k, float v){ values[k]=v; }
    float Get(const std::string& k, float def=0.f) const {
        auto it=values.find(k); return it!=values.end()?it->second:def;
    }
};

class AIDecisionMaker {
public:
    AIDecisionMaker();
    ~AIDecisionMaker();

    void Init    ();
    void Shutdown();
    void Reset   ();
    void Tick    (float dt);

    // Goal & action registry
    void RegisterGoal  (const std::string& name, float priority=1.f);
    void RegisterAction(const std::string& name, const std::string& goalName,
                        std::function<float(uint32_t agentId, const AIBlackboard&)> scorer);

    // Blackboard (global shared)
    void  SetValue(const std::string& key, float value);
    float GetValue(const std::string& key, float def=0.f) const;

    // Decision making
    std::string SelectBestAction(uint32_t agentId) const;
    void        ExecuteAction   (uint32_t agentId, const std::string& actionName);

    // Forward planning (greedy)
    std::vector<std::string> Plan(uint32_t agentId, uint32_t depth=3) const;

    // Execution binding
    void SetOnExecute(const std::string& actionName,
                      std::function<void(uint32_t agentId)> cb);

    // Cooldowns
    void SetCooldown(const std::string& actionName, float seconds);

    // Per-agent context (opaque, caller managed)
    void  SetAgentContext(uint32_t agentId, void* ctx);
    void* GetAgentContext(uint32_t agentId) const;

    // Debug
    std::string GetLastAction (uint32_t agentId) const;
    std::string DebugDump     (uint32_t agentId) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
