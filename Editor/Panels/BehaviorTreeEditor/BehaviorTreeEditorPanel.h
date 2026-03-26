#pragma once
/**
 * @file BehaviorTreeEditorPanel.h
 * @brief Visual editor panel for creating and editing AI Behavior Trees.
 *
 * Rendered using Atlas custom UI (no ImGui). Supports:
 *   - Drag-and-drop node placement (Selector, Sequence, Leaf, Decorator)
 *   - Connect nodes by dragging from parent output to child input
 *   - Inline property editing for leaf node actions/conditions
 *   - Live run-state overlay (which node is currently ticking) when PIE active
 *   - JSON save/load
 *
 * Typical usage:
 * @code
 *   BehaviorTreeEditorPanel panel;
 *   panel.Init();
 *   panel.LoadTree("AI/npc_idle.bt.json");
 *   // Each frame:
 *   panel.Tick(dt);
 *   panel.Draw(x, y, w, h);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Editor {

// ── Node kinds ────────────────────────────────────────────────────────────────

enum class BTNodeKind : uint8_t {
    Root = 0,
    Selector,        ///< runs children until one succeeds
    Sequence,        ///< runs children until one fails
    Parallel,        ///< runs all children simultaneously
    Decorator,       ///< single child + condition modifier
    Action,          ///< leaf: executes a game action
    Condition,       ///< leaf: evaluates a boolean condition
};

// ── A behavior tree node ──────────────────────────────────────────────────────

struct BTNode {
    uint32_t    id{0};
    BTNodeKind  kind{BTNodeKind::Action};
    std::string label;
    std::string actionOrCondition;  ///< identifier for Action/Condition nodes
    float       posX{0.f}, posY{0.f};
    uint32_t    parentId{0};       ///< 0 = root
    std::vector<uint32_t> childIds;
    bool        isRunning{false};  ///< live PIE overlay state
    bool        lastSucceeded{false};
};

// ── Live run-state (set from game thread) ─────────────────────────────────────

struct BTRunState {
    uint32_t activeNodeId{0};
    bool     running{false};
};

// ── BehaviorTreeEditorPanel ───────────────────────────────────────────────────

class BehaviorTreeEditorPanel {
public:
    BehaviorTreeEditorPanel();
    ~BehaviorTreeEditorPanel();

    void Init();
    void Shutdown();

    // ── Tree management ───────────────────────────────────────────────────────

    uint32_t AddNode(BTNodeKind kind, uint32_t parentId = 0,
                     const std::string& label = "");
    void     RemoveNode(uint32_t nodeId);
    void     MoveNode(uint32_t nodeId, float x, float y);
    void     SetNodeAction(uint32_t nodeId, const std::string& action);
    void     ReparentNode(uint32_t nodeId, uint32_t newParentId);

    BTNode   GetNode(uint32_t nodeId) const;
    std::vector<BTNode> AllNodes() const;

    // ── Serialization ─────────────────────────────────────────────────────────

    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);
    void Clear();

    // ── Live state ────────────────────────────────────────────────────────────

    void SetRunState(const BTRunState& state);
    void ClearRunState();

    // ── Render / input ────────────────────────────────────────────────────────

    void Draw(int panelX, int panelY, int panelW, int panelH);
    void Tick(float deltaSeconds);
    void OnMouseMove(int x, int y);
    void OnMouseButton(int button, bool pressed, int x, int y);
    void OnMouseScroll(float dx, float dy);
    void OnKey(int key, bool pressed);

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnNodeSelected(std::function<void(uint32_t nodeId)> cb);
    void OnTreeChanged(std::function<void()> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Editor
