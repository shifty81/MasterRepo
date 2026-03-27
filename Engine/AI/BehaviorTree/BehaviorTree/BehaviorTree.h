#pragma once
/**
 * @file BehaviorTree.h
 * @brief Behavior tree: Sequence, Selector, Parallel composites; decorators; leaf tasks.
 *
 * Features:
 *   - Node types: Sequence (AND), Selector (OR), Parallel
 *   - Decorator nodes: Inverter, Repeater, Cooldown, TimeLimit, Blackboard condition
 *   - Leaf nodes: Action (callback), Condition (predicate), Wait
 *   - Blackboard: typed key/value per-instance storage shared across nodes
 *   - Per-tick return values: Success, Failure, Running
 *   - Multiple instances share one tree definition; each instance has its own blackboard
 *   - Tree builder API: BehaviorTreeBuilder fluent interface
 *   - JSON tree definition loading
 *   - Per-node enter/exit debug events
 *   - Abort policies: LowerPriority, Self (re-evaluate subtree)
 *
 * Typical usage:
 * @code
 *   auto tree = BehaviorTreeBuilder()
 *       .Sequence("root")
 *           .Condition("canSeePlayer", [bb]{ return bb->GetBool("playerVisible"); })
 *           .Action("moveToPlayer",    [bb]{ return MoveToPlayer(); })
 *       .End()
 *       .Build();
 *   uint32_t instId = tree->CreateInstance();
 *   tree->Tick(instId, dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Engine {

// ── Return status ────────────────────────────────────────────────────────────

enum class BTStatus : uint8_t { Success=0, Failure, Running };

// ── Blackboard ───────────────────────────────────────────────────────────────

class Blackboard {
public:
    void  SetBool  (const std::string& k, bool   v)  { m_bools  [k]=v; }
    void  SetInt   (const std::string& k, int32_t v) { m_ints   [k]=v; }
    void  SetFloat (const std::string& k, float  v)  { m_floats [k]=v; }
    void  SetString(const std::string& k, const std::string& v) { m_strings[k]=v; }
    bool    GetBool  (const std::string& k, bool   def=false) const {
        auto it=m_bools.find(k); return it!=m_bools.end()?it->second:def; }
    int32_t GetInt   (const std::string& k, int32_t def=0)   const {
        auto it=m_ints.find(k);  return it!=m_ints.end()?it->second:def; }
    float   GetFloat (const std::string& k, float   def=0.f) const {
        auto it=m_floats.find(k);return it!=m_floats.end()?it->second:def;}
    std::string GetString(const std::string& k, const std::string& def="") const {
        auto it=m_strings.find(k);return it!=m_strings.end()?it->second:def;}
    bool HasKey(const std::string& k) const {
        return m_bools.count(k)||m_ints.count(k)||m_floats.count(k)||m_strings.count(k); }
    void Clear() { m_bools.clear();m_ints.clear();m_floats.clear();m_strings.clear(); }
private:
    std::unordered_map<std::string,bool>        m_bools;
    std::unordered_map<std::string,int32_t>     m_ints;
    std::unordered_map<std::string,float>       m_floats;
    std::unordered_map<std::string,std::string> m_strings;
};

// ── Forward declarations ─────────────────────────────────────────────────────

class BTNode;
class BehaviorTree;

// ── Node abstract base ───────────────────────────────────────────────────────

using BTContext = std::pair<BehaviorTree*, uint32_t>; ///< (tree, instanceId)

class BTNode {
public:
    explicit BTNode(std::string name) : m_name(std::move(name)) {}
    virtual ~BTNode() = default;
    virtual BTStatus Tick(Blackboard& bb, float dt) = 0;
    virtual void     Reset(Blackboard& bb) {}
    const std::string& Name() const { return m_name; }
    std::vector<std::shared_ptr<BTNode>> children;
protected:
    std::string m_name;
};

// ── Leaf nodes ───────────────────────────────────────────────────────────────

class BTAction : public BTNode {
public:
    using ActionFn = std::function<BTStatus(Blackboard&, float dt)>;
    BTAction(const std::string& name, ActionFn fn) : BTNode(name), m_fn(fn) {}
    BTStatus Tick(Blackboard& bb, float dt) override { return m_fn ? m_fn(bb,dt) : BTStatus::Failure; }
private:
    ActionFn m_fn;
};

class BTCondition : public BTNode {
public:
    using CondFn = std::function<bool(const Blackboard&)>;
    BTCondition(const std::string& name, CondFn fn) : BTNode(name), m_fn(fn) {}
    BTStatus Tick(Blackboard& bb, float) override {
        return (m_fn && m_fn(bb)) ? BTStatus::Success : BTStatus::Failure;
    }
private:
    CondFn m_fn;
};

class BTWait : public BTNode {
public:
    BTWait(const std::string& name, float duration) : BTNode(name), m_dur(duration) {}
    BTStatus Tick(Blackboard&, float dt) override {
        m_elapsed+=dt; return m_elapsed>=m_dur ? BTStatus::Success : BTStatus::Running;
    }
    void Reset(Blackboard&) override { m_elapsed=0.f; }
private:
    float m_dur{1.f}, m_elapsed{0.f};
};

// ── Composite nodes ──────────────────────────────────────────────────────────

class BTSequence : public BTNode {
public:
    explicit BTSequence(const std::string& n) : BTNode(n) {}
    BTStatus Tick(Blackboard& bb, float dt) override {
        for (auto& c : children) {
            auto s = c->Tick(bb, dt);
            if (s != BTStatus::Success) return s;
        }
        return BTStatus::Success;
    }
};

class BTSelector : public BTNode {
public:
    explicit BTSelector(const std::string& n) : BTNode(n) {}
    BTStatus Tick(Blackboard& bb, float dt) override {
        for (auto& c : children) {
            auto s = c->Tick(bb, dt);
            if (s != BTStatus::Failure) return s;
        }
        return BTStatus::Failure;
    }
};

class BTParallel : public BTNode {
public:
    BTParallel(const std::string& n, uint32_t successThreshold)
        : BTNode(n), m_threshold(successThreshold) {}
    BTStatus Tick(Blackboard& bb, float dt) override {
        uint32_t successes=0, failures=0;
        for (auto& c : children) {
            auto s = c->Tick(bb,dt);
            if (s==BTStatus::Success) successes++;
            if (s==BTStatus::Failure) failures++;
        }
        if (successes >= m_threshold)                      return BTStatus::Success;
        if (failures  > (uint32_t)children.size()-m_threshold) return BTStatus::Failure;
        return BTStatus::Running;
    }
private:
    uint32_t m_threshold{1};
};

// ── Decorator nodes ──────────────────────────────────────────────────────────

class BTInverter : public BTNode {
public:
    explicit BTInverter(const std::string& n) : BTNode(n) {}
    BTStatus Tick(Blackboard& bb, float dt) override {
        if (children.empty()) return BTStatus::Failure;
        auto s = children[0]->Tick(bb,dt);
        if (s==BTStatus::Success) return BTStatus::Failure;
        if (s==BTStatus::Failure) return BTStatus::Success;
        return BTStatus::Running;
    }
};

class BTRepeater : public BTNode {
public:
    BTRepeater(const std::string& n, uint32_t times) : BTNode(n), m_times(times) {}
    BTStatus Tick(Blackboard& bb, float dt) override {
        if (children.empty()) return BTStatus::Failure;
        while (m_count < m_times || m_times==0) {
            auto s = children[0]->Tick(bb,dt);
            if (s==BTStatus::Running) return BTStatus::Running;
            children[0]->Reset(bb);
            m_count++;
            if (m_times>0 && m_count>=m_times) return BTStatus::Success;
        }
        return BTStatus::Success;
    }
    void Reset(Blackboard& bb) override { m_count=0; if(!children.empty()) children[0]->Reset(bb); }
private:
    uint32_t m_times{1}, m_count{0};
};

// ── BehaviorTree ─────────────────────────────────────────────────────────────

class BehaviorTree {
public:
    BehaviorTree() = default;
    ~BehaviorTree() = default;

    void SetRoot(std::shared_ptr<BTNode> root) { m_root=root; }
    const std::shared_ptr<BTNode>& Root() const { return m_root; }

    uint32_t   CreateInstance();
    void       DestroyInstance(uint32_t id);
    BTStatus   Tick(uint32_t instanceId, float dt);
    void       Reset(uint32_t instanceId);
    Blackboard& GetBlackboard(uint32_t instanceId);

    bool LoadFromJSON(const std::string& path);

    using TickCb = std::function<void(const std::string& nodeName, BTStatus)>;
    void SetOnNodeTick(TickCb cb) { m_onTick=cb; }

private:
    struct Instance {
        uint32_t   id{0};
        Blackboard bb;
    };
    std::shared_ptr<BTNode>   m_root;
    std::vector<Instance>     m_instances;
    uint32_t                  m_nextId{1};
    TickCb                    m_onTick;
};

// ── Builder ───────────────────────────────────────────────────────────────────

class BehaviorTreeBuilder {
public:
    BehaviorTreeBuilder& Sequence  (const std::string& name);
    BehaviorTreeBuilder& Selector  (const std::string& name);
    BehaviorTreeBuilder& Parallel  (const std::string& name, uint32_t threshold=1);
    BehaviorTreeBuilder& Inverter  (const std::string& name);
    BehaviorTreeBuilder& Repeater  (const std::string& name, uint32_t times=0);
    BehaviorTreeBuilder& Action    (const std::string& name, BTAction::ActionFn fn);
    BehaviorTreeBuilder& Condition (const std::string& name, BTCondition::CondFn fn);
    BehaviorTreeBuilder& Wait      (const std::string& name, float duration);
    BehaviorTreeBuilder& End       ();
    std::shared_ptr<BehaviorTree>  Build();
private:
    std::vector<std::shared_ptr<BTNode>> m_stack;
    std::shared_ptr<BTNode>              m_root;
    void Push(std::shared_ptr<BTNode> node);
};

} // namespace Engine
