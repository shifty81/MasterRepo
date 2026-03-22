#include "UI/GUISystem/UILogicGraph.h"
#include <algorithm>
#include <queue>

namespace UI {

UILogicNodeID UILogicGraph::AddNode(std::unique_ptr<UILogicNode> node) {
    UILogicNodeID id = m_nextID++;
    m_nodes[id] = std::move(node);
    m_compiled  = false;
    return id;
}

void UILogicGraph::RemoveNode(UILogicNodeID id) {
    m_nodes.erase(id);
    m_edges.erase(
        std::remove_if(m_edges.begin(), m_edges.end(),
            [id](const UILogicEdge& e){ return e.fromNode == id || e.toNode == id; }),
        m_edges.end());
    m_compiled = false;
}

void UILogicGraph::AddEdge(const UILogicEdge& e)  { m_edges.push_back(e); m_compiled = false; }

void UILogicGraph::RemoveEdge(const UILogicEdge& edge) {
    m_edges.erase(
        std::remove_if(m_edges.begin(), m_edges.end(),
            [&edge](const UILogicEdge& e){
                return e.fromNode == edge.fromNode && e.fromPort == edge.fromPort &&
                       e.toNode   == edge.toNode   && e.toPort   == edge.toPort;
            }),
        m_edges.end());
    m_compiled = false;
}

bool UILogicGraph::HasCycle() const {
    std::unordered_map<UILogicNodeID, int> inDeg;
    for (auto& [id, _] : m_nodes) inDeg[id] = 0;
    for (auto& e : m_edges) if (inDeg.count(e.toNode)) inDeg[e.toNode]++;

    std::queue<UILogicNodeID> q;
    for (auto& [id, d] : inDeg) if (d == 0) q.push(id);
    int visited = 0;
    while (!q.empty()) {
        auto n = q.front(); q.pop(); ++visited;
        for (auto& e : m_edges) if (e.fromNode == n && --inDeg[e.toNode] == 0) q.push(e.toNode);
    }
    return visited != static_cast<int>(m_nodes.size());
}

bool UILogicGraph::ValidateEdgeTypes() const {
    for (auto& e : m_edges) {
        auto fi = m_nodes.find(e.fromNode), ti = m_nodes.find(e.toNode);
        if (fi == m_nodes.end() || ti == m_nodes.end()) return false;
        auto outs = fi->second->Outputs(), ins = ti->second->Inputs();
        if (e.fromPort >= outs.size() || e.toPort >= ins.size()) return false;
        if (outs[e.fromPort].type != ins[e.toPort].type) return false;
    }
    return true;
}

bool UILogicGraph::Compile() {
    m_compiled = false; m_executionOrder.clear();
    if (HasCycle() || !ValidateEdgeTypes()) return false;

    std::unordered_map<UILogicNodeID, int> inDeg;
    for (auto& [id, _] : m_nodes) inDeg[id] = 0;
    for (auto& e : m_edges) inDeg[e.toNode]++;

    std::queue<UILogicNodeID> q;
    for (auto& [id, d] : inDeg) if (d == 0) q.push(id);
    while (!q.empty()) {
        auto n = q.front(); q.pop(); m_executionOrder.push_back(n);
        for (auto& e : m_edges) if (e.fromNode == n && --inDeg[e.toNode] == 0) q.push(e.toNode);
    }
    m_compiled = (m_executionOrder.size() == m_nodes.size());
    return m_compiled;
}

bool UILogicGraph::Execute(const UILogicContext& ctx) {
    if (!m_compiled) return false;
    m_outputs.clear();
    for (UILogicNodeID id : m_executionOrder) {
        auto it = m_nodes.find(id); if (it == m_nodes.end()) return false;
        UILogicNode* node = it->second.get();
        auto inputDefs = node->Inputs();
        std::vector<UILogicValue> inputs(inputDefs.size());
        for (auto& e : m_edges) {
            if (e.toNode == id && e.toPort < inputs.size()) {
                uint64_t key = (static_cast<uint64_t>(e.fromNode) << 32) | e.fromPort;
                auto oit = m_outputs.find(key);
                if (oit != m_outputs.end()) inputs[e.toPort] = oit->second;
            }
        }
        std::vector<UILogicValue> outputs(node->Outputs().size());
        node->Evaluate(ctx, inputs, outputs);
        for (UILogicPortID p = 0; p < static_cast<UILogicPortID>(outputs.size()); ++p) {
            uint64_t key = (static_cast<uint64_t>(id) << 32) | p;
            m_outputs[key] = std::move(outputs[p]);
        }
    }
    return true;
}

const UILogicValue* UILogicGraph::GetOutput(UILogicNodeID node, UILogicPortID port) const {
    auto it = m_outputs.find((static_cast<uint64_t>(node) << 32) | port);
    return it != m_outputs.end() ? &it->second : nullptr;
}

size_t UILogicGraph::NodeCount()  const { return m_nodes.size(); }
bool   UILogicGraph::IsCompiled() const { return m_compiled; }

} // namespace UI
