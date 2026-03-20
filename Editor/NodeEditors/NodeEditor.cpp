#include "Editor/NodeEditors/NodeEditor.h"
#include <algorithm>

namespace Editor {

uint32_t NodeEditor::AddNode(const std::string& type, const std::string& label,
                              float x, float y) {
    GraphNode node;
    node.id    = m_nextNodeId++;
    node.type  = type;
    node.label = label;
    node.x     = x;
    node.y     = y;
    m_nodes.push_back(std::move(node));
    return m_nodes.back().id;
}

bool NodeEditor::RemoveNode(uint32_t id) {
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
                           [id](const GraphNode& n) { return n.id == id; });
    if (it == m_nodes.end()) return false;
    m_nodes.erase(it);
    // Remove connections referencing this node
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
                       [id](const NodeConnection& c) {
                           return c.fromNodeId == id || c.toNodeId == id;
                       }),
        m_connections.end());
    if (m_selectedNodeId == id) m_selectedNodeId = 0;
    return true;
}

uint32_t NodeEditor::AddPin(uint32_t nodeId, const std::string& name, bool isOutput) {
    for (auto& node : m_nodes) {
        if (node.id == nodeId) {
            NodePin pin;
            pin.id       = m_nextPinId++;
            pin.name     = name;
            pin.isOutput = isOutput;
            node.pins.push_back(pin);
            return pin.id;
        }
    }
    return 0;
}

uint32_t NodeEditor::Connect(uint32_t fromNode, uint32_t fromPin,
                              uint32_t toNode,   uint32_t toPin) {
    NodeConnection conn;
    conn.id         = m_nextConnectionId++;
    conn.fromNodeId = fromNode;
    conn.fromPinId  = fromPin;
    conn.toNodeId   = toNode;
    conn.toPinId    = toPin;
    m_connections.push_back(conn);
    return conn.id;
}

bool NodeEditor::Disconnect(uint32_t connectionId) {
    auto it = std::find_if(m_connections.begin(), m_connections.end(),
                           [connectionId](const NodeConnection& c) {
                               return c.id == connectionId;
                           });
    if (it == m_connections.end()) return false;
    m_connections.erase(it);
    return true;
}

const GraphNode* NodeEditor::GetNode(uint32_t id) const {
    for (const auto& node : m_nodes)
        if (node.id == id) return &node;
    return nullptr;
}

const std::vector<GraphNode>& NodeEditor::Nodes() const { return m_nodes; }
const std::vector<NodeConnection>& NodeEditor::Connections() const { return m_connections; }

void NodeEditor::Clear() {
    m_nodes.clear();
    m_connections.clear();
    m_selectedNodeId = 0;
}

void NodeEditor::SelectNode(uint32_t id)  { m_selectedNodeId = id; }
void NodeEditor::DeselectAll()            { m_selectedNodeId = 0; }
uint32_t NodeEditor::SelectedNode() const { return m_selectedNodeId; }

} // namespace Editor
