#pragma once
#include "PCG/Rules/PCGRule.h"
#include "PCG/Geometry/MeshNode.h"
#include <cstdint>
#include <string>
#include <vector>

namespace Editor {

struct PCGEditorNode {
    uint32_t      id       = 0;
    std::string   ruleName;
    PCG::RuleType ruleType = PCG::RuleType::Placement;
    float         posX     = 0.f;
    float         posY     = 0.f;
    bool          selected = false;
};

struct PCGEditorEdge {
    uint32_t    fromNodeId = 0;
    uint32_t    toNodeId   = 0;
    std::string label;
};

class PCGEditorPanel {
public:
    uint32_t AddRuleNode(PCG::PCGRule rule, float x, float y);
    void     RemoveNode(uint32_t id);

    bool ConnectNodes(uint32_t from, uint32_t to, const std::string& label);
    void DisconnectNodes(uint32_t from, uint32_t to);

    void     SelectNode(uint32_t id);
    void     DeselectAll();
    uint32_t GetSelectedId() const;

    void MoveNode(uint32_t id, float x, float y);

    bool GenerateFromGraph(PCG::PCGSeed& seed);

    PCG::PCGRuleEngine&       GetRuleEngine()       { return m_ruleEngine; }
    const PCG::PCGRuleEngine& GetRuleEngine() const { return m_ruleEngine; }

    const std::vector<PCGEditorNode>& GetNodes() const { return m_nodes; }
    const std::vector<PCGEditorEdge>& GetEdges() const { return m_edges; }

    bool Save(const std::string& path) const;
    bool Load(const std::string& path);
    void Clear();

private:
    std::vector<PCGEditorNode> m_nodes;
    std::vector<PCGEditorEdge> m_edges;
    PCG::PCGRuleEngine         m_ruleEngine;
    uint32_t                   m_nextId     = 1;
    uint32_t                   m_selectedId = 0;
};

} // namespace Editor
