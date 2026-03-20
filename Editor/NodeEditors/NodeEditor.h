#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Editor {

struct NodePin {
    uint32_t    id       = 0;
    std::string name;
    bool        isOutput = false;
};

struct GraphNode {
    uint32_t             id = 0;
    std::string          type;
    std::string          label;
    float                x = 0.f;
    float                y = 0.f;
    std::vector<NodePin> pins;
};

struct NodeConnection {
    uint32_t id        = 0;
    uint32_t fromNodeId = 0;
    uint32_t fromPinId  = 0;
    uint32_t toNodeId   = 0;
    uint32_t toPinId    = 0;
};

class NodeEditor {
public:
    NodeEditor() = default;

    uint32_t AddNode(const std::string& type, const std::string& label,
                     float x = 0.f, float y = 0.f);
    bool     RemoveNode(uint32_t id);

    uint32_t AddPin(uint32_t nodeId, const std::string& name, bool isOutput);

    uint32_t Connect(uint32_t fromNode, uint32_t fromPin,
                     uint32_t toNode,   uint32_t toPin);
    bool     Disconnect(uint32_t connectionId);

    const GraphNode*               GetNode(uint32_t id) const;
    const std::vector<GraphNode>&  Nodes() const;
    const std::vector<NodeConnection>& Connections() const;

    void Clear();

    void     SelectNode(uint32_t id);
    void     DeselectAll();
    uint32_t SelectedNode() const;

private:
    std::vector<GraphNode>       m_nodes;
    std::vector<NodeConnection>  m_connections;
    uint32_t m_nextNodeId       = 1;
    uint32_t m_nextPinId        = 1;
    uint32_t m_nextConnectionId = 1;
    uint32_t m_selectedNodeId   = 0;
};

} // namespace Editor
