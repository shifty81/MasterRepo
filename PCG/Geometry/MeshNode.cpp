#include "PCG/Geometry/MeshNode.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>

namespace PCG {

// ---- MeshNodeGraph ---------------------------------------------------------

MeshNodeID MeshNodeGraph::AddNode(std::unique_ptr<IMeshNode> node) {
    MeshNodeID id = m_nextID++;
    m_nodes[id] = std::move(node);
    m_compiled = false;
    return id;
}

void MeshNodeGraph::RemoveNode(MeshNodeID id) {
    m_nodes.erase(id);
    m_edges.erase(std::remove_if(m_edges.begin(), m_edges.end(),
        [id](const MeshEdge& e){ return e.fromNode == id || e.toNode == id; }),
        m_edges.end());
    m_compiled = false;
}

void MeshNodeGraph::AddEdge(MeshNodeID from, MeshPortID fromPort,
                            MeshNodeID to,   MeshPortID toPort) {
    m_edges.push_back({from, fromPort, to, toPort});
    m_compiled = false;
}

void MeshNodeGraph::RemoveEdge(MeshNodeID from, MeshPortID fromPort,
                               MeshNodeID to,   MeshPortID toPort) {
    m_edges.erase(std::remove_if(m_edges.begin(), m_edges.end(),
        [&](const MeshEdge& e){
            return e.fromNode == from && e.fromPort == fromPort &&
                   e.toNode == to   && e.toPort   == toPort;
        }), m_edges.end());
}

bool MeshNodeGraph::HasCycle() const {
    std::unordered_map<MeshNodeID, int> color;
    for (auto& [id, _] : m_nodes) color[id] = 0;
    std::function<bool(MeshNodeID)> dfs = [&](MeshNodeID n) -> bool {
        color[n] = 1;
        for (auto& e : m_edges) {
            if (e.fromNode == n) {
                if (color[e.toNode] == 1) return true;
                if (color[e.toNode] == 0 && dfs(e.toNode)) return true;
            }
        }
        color[n] = 2;
        return false;
    };
    for (auto& [id, _] : m_nodes)
        if (color[id] == 0 && dfs(id)) return true;
    return false;
}

bool MeshNodeGraph::ValidateEdgeTypes() const { return true; }

bool MeshNodeGraph::Compile() {
    if (HasCycle()) return false;
    m_executionOrder.clear();
    std::unordered_map<MeshNodeID, int> inDegree;
    for (auto& [id, _] : m_nodes) inDegree[id] = 0;
    for (auto& e : m_edges) inDegree[e.toNode]++;
    std::vector<MeshNodeID> queue;
    for (auto& [id, deg] : inDegree) if (deg == 0) queue.push_back(id);
    while (!queue.empty()) {
        MeshNodeID n = queue.back(); queue.pop_back();
        m_executionOrder.push_back(n);
        for (auto& e : m_edges) {
            if (e.fromNode == n && --inDegree[e.toNode] == 0)
                queue.push_back(e.toNode);
        }
    }
    m_compiled = (m_executionOrder.size() == m_nodes.size());
    return m_compiled;
}

bool MeshNodeGraph::Execute(const MeshGenContext& ctx) {
    if (!m_compiled) return false;
    m_outputs.clear();
    for (MeshNodeID nid : m_executionOrder) {
        auto it = m_nodes.find(nid);
        if (it == m_nodes.end()) continue;
        auto inPorts = it->second->Inputs();
        std::vector<MeshValue> inputs(inPorts.size());
        for (size_t pi = 0; pi < inPorts.size(); ++pi) {
            for (auto& e : m_edges) {
                if (e.toNode == nid && e.toPort == (MeshPortID)pi) {
                    uint64_t key = ((uint64_t)e.fromNode << 16) | e.fromPort;
                    auto oit = m_outputs.find(key);
                    if (oit != m_outputs.end()) inputs[pi] = oit->second;
                }
            }
        }
        auto outPorts = it->second->Outputs();
        std::vector<MeshValue> outputs(outPorts.size());
        it->second->Evaluate(ctx, inputs, outputs);
        for (size_t pi = 0; pi < outputs.size(); ++pi) {
            uint64_t key = ((uint64_t)nid << 16) | (MeshPortID)pi;
            m_outputs[key] = outputs[pi];
        }
    }
    return true;
}

const MeshValue* MeshNodeGraph::GetOutput(MeshNodeID nodeId, MeshPortID portId) const {
    uint64_t key = ((uint64_t)nodeId << 16) | portId;
    auto it = m_outputs.find(key);
    return it != m_outputs.end() ? &it->second : nullptr;
}

size_t MeshNodeGraph::NodeCount()  const { return m_nodes.size(); }
bool   MeshNodeGraph::IsCompiled() const { return m_compiled; }

// ---- HullGeneratorNode -----------------------------------------------------

std::vector<MeshPort> HullGeneratorNode::Inputs()  const {
    return { {"SizeX", MeshPinType::Float}, {"SizeY", MeshPinType::Float}, {"SizeZ", MeshPinType::Float} };
}
std::vector<MeshPort> HullGeneratorNode::Outputs() const {
    return { {"Mesh", MeshPinType::Mesh} };
}

void HullGeneratorNode::Evaluate(const MeshGenContext& ctx,
                                  const std::vector<MeshValue>& inputs,
                                  std::vector<MeshValue>& outputs) const {
    float sx = inputs.size() > 0 && !inputs[0].floatData.empty() ? inputs[0].floatData[0] : 1.0f;
    float sy = inputs.size() > 1 && !inputs[1].floatData.empty() ? inputs[1].floatData[0] : 1.0f;
    float sz = inputs.size() > 2 && !inputs[2].floatData.empty() ? inputs[2].floatData[0] : 1.0f;
    sx *= ctx.scale; sy *= ctx.scale; sz *= ctx.scale;

    MeshValue out; out.type = MeshPinType::Mesh;
    // 8 box vertices (x,y,z)
    float verts[] = {
        -sx,-sy,-sz,  sx,-sy,-sz,  sx, sy,-sz, -sx, sy,-sz,
        -sx,-sy, sz,  sx,-sy, sz,  sx, sy, sz, -sx, sy, sz
    };
    out.vertices.assign(verts, verts + 24);
    uint32_t idx[] = {
        0,1,2, 0,2,3, 4,6,5, 4,7,6,
        0,5,1, 0,4,5, 2,6,7, 2,7,3,
        0,3,7, 0,7,4, 1,5,6, 1,6,2
    };
    out.indices.assign(idx, idx + 36);
    outputs.resize(1); outputs[0] = std::move(out);
}

// ---- SubdivideNode ---------------------------------------------------------

std::vector<MeshPort> SubdivideNode::Inputs()  const { return { {"Mesh", MeshPinType::Mesh}, {"Iterations", MeshPinType::Int} }; }
std::vector<MeshPort> SubdivideNode::Outputs() const { return { {"Mesh", MeshPinType::Mesh} }; }

void SubdivideNode::Evaluate(const MeshGenContext&,
                              const std::vector<MeshValue>& inputs,
                              std::vector<MeshValue>& outputs) const {
    outputs.resize(1);
    outputs[0] = inputs.empty() ? MeshValue{} : inputs[0]; // pass-through stub
}

// ---- NoiseDeformNode -------------------------------------------------------

std::vector<MeshPort> NoiseDeformNode::Inputs()  const {
    return { {"Mesh", MeshPinType::Mesh}, {"Amplitude", MeshPinType::Float}, {"Frequency", MeshPinType::Float} };
}
std::vector<MeshPort> NoiseDeformNode::Outputs() const { return { {"Mesh", MeshPinType::Mesh} }; }

void NoiseDeformNode::Evaluate(const MeshGenContext& ctx,
                                const std::vector<MeshValue>& inputs,
                                std::vector<MeshValue>& outputs) const {
    outputs.resize(1);
    if (inputs.empty()) { outputs[0] = {}; return; }
    MeshValue out = inputs[0];
    float amp  = inputs.size() > 1 && !inputs[1].floatData.empty() ? inputs[1].floatData[0] : 0.1f;
    float freq = inputs.size() > 2 && !inputs[2].floatData.empty() ? inputs[2].floatData[0] : 1.0f;
    uint64_t s = ctx.seed ^ 0xdeadbeef;
    for (size_t i = 0; i + 2 < out.vertices.size(); i += 3) {
        s ^= s << 13; s ^= s >> 7; s ^= s << 17;
        float noise = (float)(s & 0xffff) / 65535.0f * 2.0f - 1.0f;
        out.vertices[i]   += noise * amp * std::cos(out.vertices[i]   * freq);
        out.vertices[i+1] += noise * amp * std::cos(out.vertices[i+1] * freq);
        out.vertices[i+2] += noise * amp * std::cos(out.vertices[i+2] * freq);
    }
    outputs[0] = std::move(out);
}

} // namespace PCG
