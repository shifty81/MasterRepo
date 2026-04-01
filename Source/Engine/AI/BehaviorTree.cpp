#include "Engine/AI/BehaviorTree.h"

namespace NF {

NodeStatus BTSequence::Tick(float dt) {
    while (m_CurrentChild < m_Children.size()) {
        const NodeStatus status = m_Children[m_CurrentChild]->Tick(dt);
        if (status == NodeStatus::Failure) {
            m_CurrentChild = 0;   // reset for next activation
            return NodeStatus::Failure;
        }
        if (status == NodeStatus::Running)
            return NodeStatus::Running;
        // Success: advance to next child.
        ++m_CurrentChild;
    }
    m_CurrentChild = 0;
    return NodeStatus::Success;
}

NodeStatus BTSelector::Tick(float dt) {
    while (m_CurrentChild < m_Children.size()) {
        const NodeStatus status = m_Children[m_CurrentChild]->Tick(dt);
        if (status == NodeStatus::Success) {
            m_CurrentChild = 0;
            return NodeStatus::Success;
        }
        if (status == NodeStatus::Running)
            return NodeStatus::Running;
        // Failure: try next child.
        ++m_CurrentChild;
    }
    m_CurrentChild = 0;
    return NodeStatus::Failure;
}

NodeStatus BehaviorTree::Tick(float dt) {
    if (!m_Root) return NodeStatus::Failure;
    return m_Root->Tick(dt);
}

} // namespace NF
