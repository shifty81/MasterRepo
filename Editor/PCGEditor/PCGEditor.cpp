#include "Editor/PCGEditor/PCGEditor.h"
#include <algorithm>
#include <fstream>
#include <map>

namespace Editor {

uint32_t PCGEditorPanel::AddRuleNode(PCG::PCGRule rule, float x, float y) {
    uint32_t nodeId = m_nextId++;
    rule.id = m_ruleEngine.AddRule(rule);

    PCGEditorNode n;
    n.id       = nodeId;
    n.ruleName = rule.name;
    n.ruleType = rule.type;
    n.posX     = x;
    n.posY     = y;
    n.selected = false;
    m_nodes.push_back(n);
    return nodeId;
}

void PCGEditorPanel::RemoveNode(uint32_t id) {
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [id](const PCGEditorNode& n){ return n.id == id; });
    if (it != m_nodes.end()) {
        m_ruleEngine.RemoveRule(it->id);
        m_nodes.erase(it);
    }
    m_edges.erase(std::remove_if(m_edges.begin(), m_edges.end(),
        [id](const PCGEditorEdge& e){ return e.fromNodeId == id || e.toNodeId == id; }),
        m_edges.end());
    if (m_selectedId == id) m_selectedId = 0;
}

bool PCGEditorPanel::ConnectNodes(uint32_t from, uint32_t to, const std::string& label) {
    auto hasFrom = std::any_of(m_nodes.begin(), m_nodes.end(), [from](const PCGEditorNode& n){ return n.id == from; });
    auto hasTo   = std::any_of(m_nodes.begin(), m_nodes.end(), [to]  (const PCGEditorNode& n){ return n.id == to;   });
    if (!hasFrom || !hasTo || from == to) return false;
    for (const auto& e : m_edges)
        if (e.fromNodeId == from && e.toNodeId == to) return false;
    m_edges.push_back({ from, to, label });
    return true;
}

void PCGEditorPanel::DisconnectNodes(uint32_t from, uint32_t to) {
    m_edges.erase(std::remove_if(m_edges.begin(), m_edges.end(),
        [from, to](const PCGEditorEdge& e){ return e.fromNodeId == from && e.toNodeId == to; }),
        m_edges.end());
}

void PCGEditorPanel::SelectNode(uint32_t id) {
    m_selectedId = id;
    for (auto& n : m_nodes) n.selected = (n.id == id);
}

void PCGEditorPanel::DeselectAll() {
    m_selectedId = 0;
    for (auto& n : m_nodes) n.selected = false;
}

uint32_t PCGEditorPanel::GetSelectedId() const { return m_selectedId; }

void PCGEditorPanel::MoveNode(uint32_t id, float x, float y) {
    for (auto& n : m_nodes) if (n.id == id) { n.posX = x; n.posY = y; break; }
}

bool PCGEditorPanel::GenerateFromGraph(PCG::PCGSeed& seed) {
    std::map<uint32_t, int> inDegree;
    for (const auto& n : m_nodes) inDegree[n.id] = 0;
    for (const auto& e : m_edges) inDegree[e.toNodeId]++;

    std::vector<uint32_t> queue;
    for (const auto& [id, deg] : inDegree) if (deg == 0) queue.push_back(id);

    std::map<std::string, float> context;
    bool allPassed = true;

    while (!queue.empty()) {
        uint32_t cur = queue.back(); queue.pop_back();
        if (!m_ruleEngine.Evaluate(seed, context)) { allPassed = false; }
        for (const auto& e : m_edges) {
            if (e.fromNodeId == cur) {
                if (--inDegree[e.toNodeId] == 0)
                    queue.push_back(e.toNodeId);
            }
        }
    }
    return allPassed;
}

bool PCGEditorPanel::Save(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    f << "pcgednodes " << m_nodes.size() << "\n";
    for (const auto& n : m_nodes)
        f << "node " << n.id << " " << (int)n.ruleType << " " << n.posX << " " << n.posY << " " << n.ruleName << "\n";
    f << "pcgedges " << m_edges.size() << "\n";
    for (const auto& e : m_edges)
        f << "edge " << e.fromNodeId << " " << e.toNodeId << " " << e.label << "\n";
    return true;
}

bool PCGEditorPanel::Load(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    Clear();
    std::string token;
    int count = 0;
    f >> token >> count;
    for (int i = 0; i < count; ++i) {
        PCGEditorNode n;
        int ruleType;
        f >> token >> n.id >> ruleType >> n.posX >> n.posY >> n.ruleName;
        n.ruleType = (PCG::RuleType)ruleType;
        if (n.id >= m_nextId) m_nextId = n.id + 1;
        m_nodes.push_back(n);
    }
    f >> token >> count;
    for (int i = 0; i < count; ++i) {
        PCGEditorEdge e;
        f >> token >> e.fromNodeId >> e.toNodeId >> e.label;
        m_edges.push_back(e);
    }
    return true;
}

void PCGEditorPanel::Clear() {
    m_nodes.clear();
    m_edges.clear();
    m_selectedId = 0;
    m_nextId     = 1;
}

} // namespace Editor
