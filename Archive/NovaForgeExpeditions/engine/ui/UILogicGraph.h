#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace atlas::ui {

enum class UILogicPinType : uint8_t {
    Bool,
    Float,
    Int,
    String,
    Signal
};

struct UILogicValue {
    UILogicPinType type;
    std::vector<float> data;
    std::string text;
};

struct UILogicPort {
    std::string name;
    UILogicPinType type;
};

using UILogicNodeID = uint32_t;
using UILogicPortID = uint16_t;

struct UILogicEdge {
    UILogicNodeID fromNode;
    UILogicPortID fromPort;
    UILogicNodeID toNode;
    UILogicPortID toPort;
};

struct UILogicContext {
    uint32_t tick;
    float deltaTime;
};

class UILogicNode {
public:
    virtual ~UILogicNode() = default;
    virtual const char* GetName() const = 0;
    virtual const char* GetCategory() const = 0;
    virtual std::vector<UILogicPort> Inputs() const = 0;
    virtual std::vector<UILogicPort> Outputs() const = 0;
    virtual void Evaluate(const UILogicContext& ctx,
                          const std::vector<UILogicValue>& inputs,
                          std::vector<UILogicValue>& outputs) const = 0;
};

class UILogicGraph {
public:
    UILogicNodeID AddNode(std::unique_ptr<UILogicNode> node);
    void RemoveNode(UILogicNodeID id);
    void AddEdge(const UILogicEdge& edge);
    void RemoveEdge(const UILogicEdge& edge);

    bool Compile();
    bool Execute(const UILogicContext& ctx);

    const UILogicValue* GetOutput(UILogicNodeID node, UILogicPortID port) const;
    size_t NodeCount() const;
    bool IsCompiled() const;

private:
    UILogicNodeID m_nextID = 1;
    std::unordered_map<UILogicNodeID, std::unique_ptr<UILogicNode>> m_nodes;
    std::vector<UILogicEdge> m_edges;
    std::vector<UILogicNodeID> m_executionOrder;
    bool m_compiled = false;

    std::unordered_map<uint64_t, UILogicValue> m_outputs;

    bool HasCycle() const;
    bool ValidateEdgeTypes() const;
};

}
