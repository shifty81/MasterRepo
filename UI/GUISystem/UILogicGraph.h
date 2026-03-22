#pragma once
/**
 * @file UILogicGraph.h
 * @brief UI logic node graph with DAG compilation and per-tick execution.
 *
 * Adapted from Nova-Forge-Expeditions (atlas::ui → UI namespace).
 *
 * UILogicGraph enables data-driven UI behaviour through a node graph:
 *   - UILogicNode: abstract base with typed input/output ports.
 *   - UILogicEdge: wires an output port of one node to an input port.
 *   - UILogicGraph: owns nodes and edges; Compile() validates the DAG and
 *     builds topological execution order; Execute(ctx) runs nodes in order.
 *
 * UILogicPinType: Bool, Float, Int, String, Signal.
 * UILogicContext: tick (uint32_t), deltaTime (float).
 * GetOutput(nodeId, port): retrieve the evaluated output value.
 */

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace UI {

// ── Pin types ─────────────────────────────────────────────────────────────
enum class UILogicPinType : uint8_t { Bool, Float, Int, String, Signal };

// ── Value ─────────────────────────────────────────────────────────────────
struct UILogicValue {
    UILogicPinType     type{UILogicPinType::Float};
    std::vector<float> data;
    std::string        text;
};

// ── Port ──────────────────────────────────────────────────────────────────
struct UILogicPort {
    std::string    name;
    UILogicPinType type{UILogicPinType::Float};
};

// ── Ids ───────────────────────────────────────────────────────────────────
using UILogicNodeID = uint32_t;
using UILogicPortID = uint16_t;

// ── Edge ──────────────────────────────────────────────────────────────────
struct UILogicEdge {
    UILogicNodeID fromNode{0};
    UILogicPortID fromPort{0};
    UILogicNodeID toNode{0};
    UILogicPortID toPort{0};
};

// ── Execution context ─────────────────────────────────────────────────────
struct UILogicContext {
    uint32_t tick{0};
    float    deltaTime{0};
};

// ── Abstract logic node ───────────────────────────────────────────────────
class UILogicNode {
public:
    virtual ~UILogicNode() = default;
    virtual const char*                GetName()     const = 0;
    virtual const char*                GetCategory() const = 0;
    virtual std::vector<UILogicPort>   Inputs()      const = 0;
    virtual std::vector<UILogicPort>   Outputs()     const = 0;
    virtual void Evaluate(const UILogicContext&           ctx,
                          const std::vector<UILogicValue>& inputs,
                          std::vector<UILogicValue>&        outputs) const = 0;
};

// ── Graph ─────────────────────────────────────────────────────────────────
class UILogicGraph {
public:
    UILogicGraph()  = default;
    ~UILogicGraph() = default;

    UILogicGraph(const UILogicGraph&) = delete;
    UILogicGraph& operator=(const UILogicGraph&) = delete;

    UILogicNodeID AddNode(std::unique_ptr<UILogicNode> node);
    void          RemoveNode(UILogicNodeID id);
    void          AddEdge(const UILogicEdge& edge);
    void          RemoveEdge(const UILogicEdge& edge);

    bool Compile();
    bool Execute(const UILogicContext& ctx);

    const UILogicValue* GetOutput(UILogicNodeID node, UILogicPortID port) const;
    size_t              NodeCount()  const;
    bool                IsCompiled() const;

private:
    bool HasCycle()          const;
    bool ValidateEdgeTypes() const;

    UILogicNodeID m_nextID{1};
    std::unordered_map<UILogicNodeID, std::unique_ptr<UILogicNode>> m_nodes;
    std::vector<UILogicEdge>                                        m_edges;
    std::vector<UILogicNodeID>                                      m_executionOrder;
    bool                                                            m_compiled{false};
    std::unordered_map<uint64_t, UILogicValue>                      m_outputs;
};

} // namespace UI
