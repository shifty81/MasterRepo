#include "Editor/MaterialEditor/MaterialEditor.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace Editor {

uint32_t MaterialGraph::AddNode(const std::string& name, const std::string& category,
                                 const std::vector<MaterialPin>& inputs,
                                 const std::vector<MaterialPin>& outputs) {
    MaterialNode node;
    node.id       = m_nextId++;
    node.name     = name;
    node.category = category;
    for (auto& p : inputs)  { MaterialPin pin = p; pin.isOutput = false; node.pins.push_back(pin); }
    for (auto& p : outputs) { MaterialPin pin = p; pin.isOutput = true;  node.pins.push_back(pin); }
    m_compiled = false;
    m_nodes.push_back(node);
    return node.id;
}

void MaterialGraph::RemoveNode(uint32_t id) {
    m_nodes.erase(std::remove_if(m_nodes.begin(), m_nodes.end(),
        [id](const MaterialNode& n){ return n.id == id; }), m_nodes.end());
    m_edges.erase(std::remove_if(m_edges.begin(), m_edges.end(),
        [id](const MaterialEdge& e){ return e.fromNodeId == id || e.toNodeId == id; }), m_edges.end());
    m_compiled = false;
}

bool MaterialGraph::AddEdge(uint32_t fromNode, uint32_t fromPin,
                             uint32_t toNode,   uint32_t toPin) {
    const MaterialNode* src = GetNode(fromNode);
    const MaterialNode* dst = GetNode(toNode);
    if (!src || !dst) return false;
    if (fromPin >= src->pins.size() || !src->pins[fromPin].isOutput) return false;
    if (toPin  >= dst->pins.size() || dst->pins[toPin].isOutput)     return false;
    if (src->pins[fromPin].type != dst->pins[toPin].type) return false;
    MaterialEdge e{ fromNode, fromPin, toNode, toPin };
    m_edges.push_back(e);
    m_compiled = false;
    return true;
}

void MaterialGraph::RemoveEdge(uint32_t fromNode, uint32_t fromPin,
                                uint32_t toNode,   uint32_t toPin) {
    m_edges.erase(std::remove_if(m_edges.begin(), m_edges.end(),
        [&](const MaterialEdge& e){
            return e.fromNodeId == fromNode && e.fromPinIdx == fromPin &&
                   e.toNodeId   == toNode   && e.toPinIdx   == toPin;
        }), m_edges.end());
    m_compiled = false;
}

MaterialNode* MaterialGraph::GetNode(uint32_t id) {
    for (auto& n : m_nodes) if (n.id == id) return &n;
    return nullptr;
}
const MaterialNode* MaterialGraph::GetNode(uint32_t id) const {
    for (const auto& n : m_nodes) if (n.id == id) return &n;
    return nullptr;
}

std::vector<MaterialEdge> MaterialGraph::GetEdgesFrom(uint32_t nodeId) const {
    std::vector<MaterialEdge> result;
    for (const auto& e : m_edges) if (e.fromNodeId == nodeId) result.push_back(e);
    return result;
}

std::vector<MaterialEdge> MaterialGraph::GetEdgesTo(uint32_t nodeId) const {
    std::vector<MaterialEdge> result;
    for (const auto& e : m_edges) if (e.toNodeId == nodeId) result.push_back(e);
    return result;
}

bool MaterialGraph::TopologicalSort(std::vector<uint32_t>& order) const {
    std::unordered_map<uint32_t, int> inDegree;
    for (const auto& n : m_nodes) inDegree[n.id] = 0;
    for (const auto& e : m_edges) inDegree[e.toNodeId]++;

    std::vector<uint32_t> queue;
    for (const auto& [id, deg] : inDegree)
        if (deg == 0) queue.push_back(id);

    while (!queue.empty()) {
        uint32_t cur = queue.back(); queue.pop_back();
        order.push_back(cur);
        for (const auto& e : m_edges) {
            if (e.fromNodeId == cur) {
                if (--inDegree[e.toNodeId] == 0)
                    queue.push_back(e.toNodeId);
            }
        }
    }
    return order.size() == m_nodes.size();
}

bool MaterialGraph::Compile() {
    std::vector<uint32_t> order;
    if (!TopologicalSort(order)) { m_compiled = false; return false; }
    m_compiled = true;
    return true;
}

bool MaterialGraph::IsCompiled() const { return m_compiled; }

bool MaterialGraph::SaveToFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    f << "nodes " << m_nodes.size() << "\n";
    for (const auto& n : m_nodes) {
        f << "node " << n.id << " " << n.category << " " << n.name << " "
          << n.x << " " << n.y << " " << n.pins.size() << "\n";
        for (const auto& p : n.pins)
            f << "pin " << p.name << " " << (int)p.type << " " << (int)p.isOutput << "\n";
    }
    f << "edges " << m_edges.size() << "\n";
    for (const auto& e : m_edges)
        f << "edge " << e.fromNodeId << " " << e.fromPinIdx << " " << e.toNodeId << " " << e.toPinIdx << "\n";
    return true;
}

bool MaterialGraph::LoadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    Clear();
    std::string token;
    int nodeCount = 0;
    f >> token >> nodeCount;
    for (int i = 0; i < nodeCount; ++i) {
        MaterialNode n;
        int pinCount = 0;
        f >> token >> n.id >> n.category >> n.name >> n.x >> n.y >> pinCount;
        for (int j = 0; j < pinCount; ++j) {
            MaterialPin p;
            int type, isOut;
            f >> token >> p.name >> type >> isOut;
            p.type     = (MaterialPinType)type;
            p.isOutput = (bool)isOut;
            n.pins.push_back(p);
        }
        if (n.id >= m_nextId) m_nextId = n.id + 1;
        m_nodes.push_back(n);
    }
    int edgeCount = 0;
    f >> token >> edgeCount;
    for (int i = 0; i < edgeCount; ++i) {
        MaterialEdge e;
        f >> token >> e.fromNodeId >> e.fromPinIdx >> e.toNodeId >> e.toPinIdx;
        m_edges.push_back(e);
    }
    return true;
}

void MaterialGraph::Clear() {
    m_nodes.clear();
    m_edges.clear();
    m_compiled = false;
    m_nextId   = 1;
}

// ─── MaterialEditorPanel ────────────────────────────────────────────────────

void MaterialEditorPanel::CreateDefaultPBR() {
    m_graph.Clear();
    MaterialPin floatPin{"", MaterialPinType::Float, false};
    MaterialPin colorPin{"", MaterialPinType::Color, false};
    MaterialPin colorOut{"", MaterialPinType::Color, true};
    MaterialPin floatOut{"", MaterialPinType::Float, true};

    floatPin.name = "value"; floatOut.name = "out";
    colorPin.name = "color"; colorOut.name = "out";

    uint32_t baseColor  = m_graph.AddNode("BaseColor",  "Input",  {colorPin}, {colorOut});
    uint32_t metallic   = m_graph.AddNode("Metallic",   "Input",  {floatPin}, {floatOut});
    uint32_t roughness  = m_graph.AddNode("Roughness",  "Input",  {floatPin}, {floatOut});

    MaterialPin cIn{"BaseColor", MaterialPinType::Color, false};
    MaterialPin mIn{"Metallic",  MaterialPinType::Float, false};
    MaterialPin rIn{"Roughness", MaterialPinType::Float, false};
    uint32_t output = m_graph.AddNode("PBR_Output", "Output", {cIn, mIn, rIn}, {});

    m_graph.AddEdge(baseColor, 0, output, 0);
    m_graph.AddEdge(metallic,  0, output, 1);
    m_graph.AddEdge(roughness, 0, output, 2);
    m_graph.Compile();

    MaterialNode* bcNode = m_graph.GetNode(baseColor);
    if (bcNode) { bcNode->x = 100.f; bcNode->y = 100.f; }
    MaterialNode* mNode = m_graph.GetNode(metallic);
    if (mNode)  { mNode->x  = 100.f; mNode->y = 250.f; }
    MaterialNode* rNode = m_graph.GetNode(roughness);
    if (rNode)  { rNode->x  = 100.f; rNode->y = 400.f; }
    MaterialNode* oNode = m_graph.GetNode(output);
    if (oNode)  { oNode->x  = 400.f; oNode->y = 200.f; }
}

void MaterialEditorPanel::SelectNode(uint32_t id)  { m_selectedNodeId = id; }
void MaterialEditorPanel::DeselectNode()            { m_selectedNodeId = 0; }
uint32_t MaterialEditorPanel::GetSelectedNodeId() const { return m_selectedNodeId; }

void MaterialEditorPanel::MoveNode(uint32_t id, float x, float y) {
    MaterialNode* n = m_graph.GetNode(id);
    if (n) { n->x = x; n->y = y; }
    // Keep visual NodeEditor in sync
    auto& nodes = const_cast<std::vector<GraphNode>&>(m_nodeEditor.Nodes());
    for (auto& gn : nodes) {
        if (gn.id == id) { gn.x = x; gn.y = y; break; }
    }
}

void MaterialEditorPanel::DeleteSelected() {
    if (m_selectedNodeId == 0) return;
    m_graph.RemoveNode(m_selectedNodeId);
    m_nodeEditor.RemoveNode(m_selectedNodeId);
    m_selectedNodeId = 0;
}

// ── PL-04: NodeEditor-backed visual graph API ──────────────────────────────

MaterialEditorPanel::MaterialEditorPanel() {
    // Default constructor — graph is empty; call CreateDefaultPBR() to populate.
}

uint32_t MaterialEditorPanel::AddVisualNode(const std::string& name,
                                             const std::string& category,
                                             const std::vector<MaterialPin>& inputs,
                                             const std::vector<MaterialPin>& outputs,
                                             float x, float y) {
    // Add to MaterialGraph (authoritative data)
    uint32_t matId = m_graph.AddNode(name, category, inputs, outputs);

    // Mirror in NodeEditor for visual display / selection
    uint32_t visId = m_nodeEditor.AddNode(category, name, x, y);
    for (const auto& p : inputs)
        m_nodeEditor.AddPin(visId, p.name, false);
    for (const auto& p : outputs)
        m_nodeEditor.AddPin(visId, p.name, true);

    // Update position in MaterialGraph node
    auto* mn = m_graph.GetNode(matId);
    if (mn) { mn->x = x; mn->y = y; }

    return matId;
}

bool MaterialEditorPanel::ConnectVisual(uint32_t fromNode, uint32_t fromPin,
                                         uint32_t toNode,   uint32_t toPin) {
    bool ok = m_graph.AddEdge(fromNode, fromPin, toNode, toPin);
    if (ok) m_nodeEditor.Connect(fromNode, fromPin, toNode, toPin);
    return ok;
}

} // namespace Editor
