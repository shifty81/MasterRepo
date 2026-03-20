#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Editor {

enum class MaterialPinType { Float, Vec2, Vec3, Vec4, Texture2D, Color, Bool };

struct MaterialPin {
    std::string    name;
    MaterialPinType type     = MaterialPinType::Float;
    bool           isOutput = false;
};

struct MaterialNode {
    uint32_t                 id       = 0;
    std::string              name;
    std::string              category;
    std::vector<MaterialPin> pins;
    float                    x = 0.f;
    float                    y = 0.f;
};

struct MaterialEdge {
    uint32_t fromNodeId = 0;
    uint32_t fromPinIdx = 0;
    uint32_t toNodeId   = 0;
    uint32_t toPinIdx   = 0;
};

class MaterialGraph {
public:
    uint32_t AddNode(const std::string& name, const std::string& category,
                     const std::vector<MaterialPin>& inputs,
                     const std::vector<MaterialPin>& outputs);
    void     RemoveNode(uint32_t id);

    bool AddEdge(uint32_t fromNode, uint32_t fromPin,
                 uint32_t toNode,   uint32_t toPin);
    void RemoveEdge(uint32_t fromNode, uint32_t fromPin,
                    uint32_t toNode,   uint32_t toPin);

    MaterialNode*       GetNode(uint32_t id);
    const MaterialNode* GetNode(uint32_t id) const;

    std::vector<MaterialEdge> GetEdgesFrom(uint32_t nodeId) const;
    std::vector<MaterialEdge> GetEdgesTo(uint32_t nodeId)   const;

    bool Compile();
    bool IsCompiled() const;

    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);

    const std::vector<MaterialNode>& GetAllNodes() const { return m_nodes; }
    const std::vector<MaterialEdge>& GetAllEdges() const { return m_edges; }
    void Clear();

private:
    bool TopologicalSort(std::vector<uint32_t>& order) const;

    uint32_t m_nextId    = 1;
    bool     m_compiled  = false;
    std::vector<MaterialNode> m_nodes;
    std::vector<MaterialEdge> m_edges;
};

class MaterialEditorPanel {
public:
    void CreateDefaultPBR();

    MaterialGraph&       GetGraph()       { return m_graph; }
    const MaterialGraph& GetGraph() const { return m_graph; }

    void     SelectNode(uint32_t id);
    void     DeselectNode();
    uint32_t GetSelectedNodeId() const;

    void MoveNode(uint32_t id, float x, float y);
    void DeleteSelected();

private:
    MaterialGraph m_graph;
    uint32_t      m_selectedNodeId = 0;
};

} // namespace Editor
