#include "Runtime/AI/BehaviorTree/BehaviorTree.h"

namespace Runtime::AI {

// ── BTNode ────────────────────────────────────────────────────────────────
BTNode::BTNode(std::string name) : m_name(std::move(name)) {}

// ── ActionNode ────────────────────────────────────────────────────────────
ActionNode::ActionNode(std::string name, ActionFn fn)
    : BTNode(std::move(name)), m_fn(std::move(fn)) {}

BTStatus ActionNode::Tick(Blackboard& bb) {
    BTStatus s = m_fn ? m_fn(bb) : BTStatus::Failure;
    m_running = (s == BTStatus::Running);
    return s;
}

// ── ConditionNode ─────────────────────────────────────────────────────────
ConditionNode::ConditionNode(std::string name, ConditionFn fn)
    : BTNode(std::move(name)), m_fn(std::move(fn)) {}

BTStatus ConditionNode::Tick(Blackboard& bb) {
    return (m_fn && m_fn(bb)) ? BTStatus::Success : BTStatus::Failure;
}

// ── CompositeNode ─────────────────────────────────────────────────────────
CompositeNode::CompositeNode(std::string name) : BTNode(std::move(name)) {}

void CompositeNode::AddChild(std::shared_ptr<BTNode> child) {
    if (child) m_children.push_back(std::move(child));
}
size_t CompositeNode::ChildCount() const { return m_children.size(); }

void CompositeNode::Reset() {
    m_currentChild = 0;
    m_running = false;
    for (auto& c : m_children) c->Reset();
}

// ── SequenceNode ──────────────────────────────────────────────────────────
SequenceNode::SequenceNode(std::string name) : CompositeNode(std::move(name)) {}

BTStatus SequenceNode::Tick(Blackboard& bb) {
    // Resume from current child if previously Running
    for (; m_currentChild < m_children.size(); ++m_currentChild) {
        BTStatus s = m_children[m_currentChild]->Tick(bb);
        if (s == BTStatus::Failure) { m_currentChild = 0; m_running = false; return BTStatus::Failure; }
        if (s == BTStatus::Running) { m_running = true;  return BTStatus::Running; }
        // Success: advance to next child
    }
    m_currentChild = 0;
    m_running = false;
    return BTStatus::Success;
}

// ── SelectorNode ──────────────────────────────────────────────────────────
SelectorNode::SelectorNode(std::string name) : CompositeNode(std::move(name)) {}

BTStatus SelectorNode::Tick(Blackboard& bb) {
    for (; m_currentChild < m_children.size(); ++m_currentChild) {
        BTStatus s = m_children[m_currentChild]->Tick(bb);
        if (s == BTStatus::Success) { m_currentChild = 0; m_running = false; return BTStatus::Success; }
        if (s == BTStatus::Running) { m_running = true;  return BTStatus::Running; }
        // Failure: try next child
    }
    m_currentChild = 0;
    m_running = false;
    return BTStatus::Failure;
}

// ── ParallelNode ──────────────────────────────────────────────────────────
ParallelNode::ParallelNode(std::string name, ParallelPolicy policy)
    : CompositeNode(std::move(name)), m_policy(policy) {}

BTStatus ParallelNode::Tick(Blackboard& bb) {
    int successes = 0;
    int failures  = 0;
    for (auto& child : m_children) {
        BTStatus s = child->Tick(bb);
        if (s == BTStatus::Success) ++successes;
        else if (s == BTStatus::Failure) ++failures;
    }
    int total = static_cast<int>(m_children.size());
    if (m_policy == ParallelPolicy::RequireAll) {
        if (failures  > 0) return BTStatus::Failure;
        if (successes == total) return BTStatus::Success;
    } else {
        if (successes > 0) return BTStatus::Success;
        if (failures  == total) return BTStatus::Failure;
    }
    return BTStatus::Running;
}

// ── DecoratorNode ─────────────────────────────────────────────────────────
DecoratorNode::DecoratorNode(std::string name, std::shared_ptr<BTNode> child)
    : BTNode(std::move(name)), m_child(std::move(child)) {}

void DecoratorNode::Reset() {
    BTNode::Reset();
    if (m_child) m_child->Reset();
}

// ── InvertNode ────────────────────────────────────────────────────────────
InvertNode::InvertNode(std::shared_ptr<BTNode> child)
    : DecoratorNode("Invert", std::move(child)) {}

BTStatus InvertNode::Tick(Blackboard& bb) {
    if (!m_child) return BTStatus::Failure;
    BTStatus s = m_child->Tick(bb);
    if (s == BTStatus::Success) return BTStatus::Failure;
    if (s == BTStatus::Failure) return BTStatus::Success;
    return BTStatus::Running;
}

// ── RepeatNode ────────────────────────────────────────────────────────────
RepeatNode::RepeatNode(std::shared_ptr<BTNode> child, uint32_t times)
    : DecoratorNode("Repeat", std::move(child)), m_times(times) {}

BTStatus RepeatNode::Tick(Blackboard& bb) {
    if (!m_child) return BTStatus::Failure;
    BTStatus s = m_child->Tick(bb);
    if (s == BTStatus::Running) return BTStatus::Running;
    // Child finished one iteration
    m_child->Reset();
    ++m_count;
    if (m_times > 0 && m_count >= m_times) {
        m_count = 0;
        return (s == BTStatus::Success) ? BTStatus::Success : BTStatus::Failure;
    }
    // Infinite or not yet reached limit — keep running
    return BTStatus::Running;
}

void RepeatNode::Reset() {
    DecoratorNode::Reset();
    m_count = 0;
}

// ── LimitNode ─────────────────────────────────────────────────────────────
LimitNode::LimitNode(std::shared_ptr<BTNode> child, uint32_t maxRuns)
    : DecoratorNode("Limit", std::move(child)), m_maxRuns(maxRuns) {}

BTStatus LimitNode::Tick(Blackboard& bb) {
    if (m_runs >= m_maxRuns) return BTStatus::Failure;
    if (!m_child) return BTStatus::Failure;
    BTStatus s = m_child->Tick(bb);
    if (s != BTStatus::Running) ++m_runs;
    return s;
}

void LimitNode::Reset() {
    DecoratorNode::Reset();
    m_runs = 0;
}

// ── BehaviorTree ─────────────────────────────────────────────────────────
BehaviorTree::BehaviorTree(std::shared_ptr<BTNode> root)
    : m_root(std::move(root)) {}

void BehaviorTree::SetRoot(std::shared_ptr<BTNode> root) {
    m_root = std::move(root);
}

BTStatus BehaviorTree::Tick() {
    if (!m_root) return BTStatus::Failure;
    m_lastStatus = m_root->Tick(m_blackboard);
    if (m_lastStatus != BTStatus::Running) m_root->Reset();
    return m_lastStatus;
}

void BehaviorTree::Reset() {
    if (m_root) m_root->Reset();
    m_lastStatus = BTStatus::Failure;
}

} // namespace Runtime::AI
