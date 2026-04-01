#pragma once
#include <functional>
#include <memory>
#include <vector>

namespace NF {

/// @brief Return status of a single behavior-tree node tick.
enum class NodeStatus {
    Running, ///< Node is still executing; tick again next frame.
    Success, ///< Node completed successfully.
    Failure  ///< Node completed with failure.
};

// ---------------------------------------------------------------------------
// Base node
// ---------------------------------------------------------------------------

/// @brief Abstract base class for all behavior tree nodes.
class BTNode {
public:
    virtual ~BTNode() = default;

    /// @brief Advance the node's logic by one tick.
    /// @param dt Elapsed seconds since the last tick.
    /// @return Current execution status.
    virtual NodeStatus Tick(float dt) = 0;
};

// ---------------------------------------------------------------------------
// Composite nodes
// ---------------------------------------------------------------------------

/// @brief Sequence node (AND): ticks children left-to-right; returns Failure
///        as soon as any child fails, Success when all children succeed.
class BTSequence : public BTNode {
public:
    /// @brief Append a child node.
    void AddChild(std::shared_ptr<BTNode> child) {
        m_Children.push_back(std::move(child));
    }

    NodeStatus Tick(float dt) override;

private:
    std::vector<std::shared_ptr<BTNode>> m_Children;
    std::size_t m_CurrentChild{0};
};

/// @brief Selector node (OR): ticks children left-to-right; returns Success
///        as soon as any child succeeds, Failure when all children fail.
class BTSelector : public BTNode {
public:
    /// @brief Append a child node.
    void AddChild(std::shared_ptr<BTNode> child) {
        m_Children.push_back(std::move(child));
    }

    NodeStatus Tick(float dt) override;

private:
    std::vector<std::shared_ptr<BTNode>> m_Children;
    std::size_t m_CurrentChild{0};
};

// ---------------------------------------------------------------------------
// Leaf node
// ---------------------------------------------------------------------------

/// @brief Leaf node whose behaviour is defined by an arbitrary callback.
class BTLeaf : public BTNode {
public:
    /// @brief Construct a leaf node from a callable.
    /// @param action Callback with signature NodeStatus(float dt).
    explicit BTLeaf(std::function<NodeStatus(float)> action)
        : m_Action(std::move(action)) {}

    NodeStatus Tick(float dt) override {
        return m_Action ? m_Action(dt) : NodeStatus::Failure;
    }

private:
    std::function<NodeStatus(float)> m_Action;
};

// ---------------------------------------------------------------------------
// BehaviorTree driver
// ---------------------------------------------------------------------------

/// @brief Thin driver that owns the root node and forwards ticks to it.
class BehaviorTree {
public:
    /// @brief Set (or replace) the root node of the tree.
    /// @param root Shared ownership of the root BTNode.
    void SetRoot(std::shared_ptr<BTNode> root) { m_Root = std::move(root); }

    /// @brief Advance the entire tree by one tick.
    /// @param dt Elapsed seconds since the last tick.
    /// @return The status returned by the root node, or NodeStatus::Failure if
    ///         no root has been set.
    NodeStatus Tick(float dt);

private:
    std::shared_ptr<BTNode> m_Root;
};

} // namespace NF
