#pragma once
/**
 * @file BehaviorTree.h
 * @brief Behavior Tree system for NPC/Agent AI decision-making.
 *
 * Provides a complete, composable BT implementation:
 *   - Leaf nodes: Action, Condition
 *   - Composites: Sequence (AND), Selector (OR), Parallel
 *   - Decorators: Invert, Repeat, Limit, Cooldown
 *   - Blackboard: typed key-value shared context per tree instance
 *   - Tick-based execution: each call to Tick() drives the tree one step
 *
 * Design follows the standard BT return-status model:
 *   Success / Failure / Running
 */

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <any>

namespace Runtime::AI {

// ── Status ────────────────────────────────────────────────────────────────
enum class BTStatus : uint8_t {
    Success,
    Failure,
    Running,
};

// ── Blackboard ────────────────────────────────────────────────────────────
class Blackboard {
public:
    template<typename T>
    void Set(const std::string& key, const T& value) { m_data[key] = value; }

    template<typename T>
    T Get(const std::string& key, const T& def = T{}) const {
        auto it = m_data.find(key);
        if (it == m_data.end()) return def;
        try { return std::any_cast<T>(it->second); } catch (...) { return def; }
    }

    bool Has(const std::string& key) const { return m_data.count(key) > 0; }
    void Remove(const std::string& key)    { m_data.erase(key); }
    void Clear()                           { m_data.clear(); }

private:
    std::unordered_map<std::string, std::any> m_data;
};

// ── Base node ─────────────────────────────────────────────────────────────
class BTNode {
public:
    explicit BTNode(std::string name = "");
    virtual ~BTNode() = default;

    const std::string& Name() const { return m_name; }

    /// Called once when the node becomes active (before first Tick).
    virtual void OnEnter(Blackboard&) {}
    /// Called once when the node returns Success or Failure.
    virtual void OnExit(Blackboard&)  {}
    /// Drive the node; must return Success, Failure, or Running.
    virtual BTStatus Tick(Blackboard& bb) = 0;
    /// Reset to initial state (clears Running state).
    virtual void Reset() { m_running = false; }

protected:
    std::string m_name;
    bool        m_running{false};
};

// ── Leaf: Action ──────────────────────────────────────────────────────────
using ActionFn = std::function<BTStatus(Blackboard&)>;

class ActionNode : public BTNode {
public:
    explicit ActionNode(std::string name, ActionFn fn);
    BTStatus Tick(Blackboard& bb) override;
private:
    ActionFn m_fn;
};

// ── Leaf: Condition ───────────────────────────────────────────────────────
using ConditionFn = std::function<bool(const Blackboard&)>;

class ConditionNode : public BTNode {
public:
    explicit ConditionNode(std::string name, ConditionFn fn);
    BTStatus Tick(Blackboard& bb) override;
private:
    ConditionFn m_fn;
};

// ── Composite base ────────────────────────────────────────────────────────
class CompositeNode : public BTNode {
public:
    explicit CompositeNode(std::string name);
    void AddChild(std::shared_ptr<BTNode> child);
    size_t ChildCount() const;
    void Reset() override;
protected:
    std::vector<std::shared_ptr<BTNode>> m_children;
    size_t m_currentChild{0};
};

// ── Sequence: ticks children in order; fails on first Failure ─────────────
class SequenceNode : public CompositeNode {
public:
    explicit SequenceNode(std::string name = "Sequence");
    BTStatus Tick(Blackboard& bb) override;
};

// ── Selector: ticks children in order; succeeds on first Success ──────────
class SelectorNode : public CompositeNode {
public:
    explicit SelectorNode(std::string name = "Selector");
    BTStatus Tick(Blackboard& bb) override;
};

// ── Parallel: ticks all children every tick ───────────────────────────────
enum class ParallelPolicy {
    RequireAll,   ///< Succeed when ALL children succeed
    RequireOne,   ///< Succeed when ANY child succeeds
};

class ParallelNode : public CompositeNode {
public:
    explicit ParallelNode(std::string name = "Parallel",
                          ParallelPolicy policy = ParallelPolicy::RequireOne);
    BTStatus Tick(Blackboard& bb) override;
private:
    ParallelPolicy m_policy;
};

// ── Decorator base ────────────────────────────────────────────────────────
class DecoratorNode : public BTNode {
public:
    explicit DecoratorNode(std::string name, std::shared_ptr<BTNode> child);
    void Reset() override;
protected:
    std::shared_ptr<BTNode> m_child;
};

// ── Invert: flips Success↔Failure ────────────────────────────────────────
class InvertNode : public DecoratorNode {
public:
    explicit InvertNode(std::shared_ptr<BTNode> child);
    BTStatus Tick(Blackboard& bb) override;
};

// ── Repeat: ticks child N times (0 = forever) ────────────────────────────
class RepeatNode : public DecoratorNode {
public:
    explicit RepeatNode(std::shared_ptr<BTNode> child, uint32_t times = 0);
    BTStatus Tick(Blackboard& bb) override;
    void Reset() override;
private:
    uint32_t m_times{0};
    uint32_t m_count{0};
};

// ── Limit: runs child at most N times per Reset cycle ────────────────────
class LimitNode : public DecoratorNode {
public:
    explicit LimitNode(std::shared_ptr<BTNode> child, uint32_t maxRuns);
    BTStatus Tick(Blackboard& bb) override;
    void Reset() override;
private:
    uint32_t m_maxRuns{1};
    uint32_t m_runs{0};
};

// ── BehaviorTree ─────────────────────────────────────────────────────────
class BehaviorTree {
public:
    explicit BehaviorTree(std::shared_ptr<BTNode> root = nullptr);

    void SetRoot(std::shared_ptr<BTNode> root);
    BTStatus Tick();
    void Reset();

    Blackboard&       GetBlackboard()       { return m_blackboard; }
    const Blackboard& GetBlackboard() const { return m_blackboard; }

    BTStatus LastStatus() const { return m_lastStatus; }

private:
    std::shared_ptr<BTNode> m_root;
    Blackboard              m_blackboard;
    BTStatus                m_lastStatus{BTStatus::Failure};
};

} // namespace Runtime::AI
