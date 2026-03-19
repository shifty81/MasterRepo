#include "LODBakingGraph.h"
#include "LODBakingNodes.h"
#include "ProceduralMeshNodes.h"
#include <algorithm>
#include <queue>
#include <cstdlib>
#include <stdexcept>

namespace atlas::procedural {

static float SafeStof(const std::string& s, float def) {
    try { return s.empty() ? def : std::stof(s); }
    catch (...) { return def; }
}

static int SafeStoi(const std::string& s, int def) {
    try { return s.empty() ? def : std::stoi(s); }
    catch (...) { return def; }
}

uint32_t LODBakingGraph::AddNode(LODNodeType type) {
    uint32_t id = m_nextID++;
    LODNode node;
    node.id = id;
    node.type = type;
    m_nodes[id] = std::move(node);
    m_compiled = false;
    return id;
}

void LODBakingGraph::SetNodeProperty(uint32_t id, const std::string& key, const std::string& value) {
    auto it = m_nodes.find(id);
    if (it == m_nodes.end()) return;
    for (auto& p : it->second.properties) {
        if (p.first == key) {
            p.second = value;
            m_compiled = false;
            return;
        }
    }
    it->second.properties.push_back({key, value});
    m_compiled = false;
}

void LODBakingGraph::AddEdge(const LODEdge& edge) {
    m_edges.push_back(edge);
    m_compiled = false;
}

void LODBakingGraph::RemoveNode(uint32_t id) {
    m_nodes.erase(id);
    m_edges.erase(
        std::remove_if(m_edges.begin(), m_edges.end(),
            [id](const LODEdge& e) {
                return e.fromNode == id || e.toNode == id;
            }),
        m_edges.end()
    );
    m_compiled = false;
}

bool LODBakingGraph::HasCycle() const {
    std::unordered_map<uint32_t, int> inDegree;
    for (auto& [id, _] : m_nodes) {
        inDegree[id] = 0;
    }
    for (auto& e : m_edges) {
        if (inDegree.count(e.toNode)) {
            inDegree[e.toNode]++;
        }
    }

    std::queue<uint32_t> q;
    for (auto& [id, deg] : inDegree) {
        if (deg == 0) q.push(id);
    }

    int visited = 0;
    while (!q.empty()) {
        uint32_t n = q.front();
        q.pop();
        visited++;
        for (auto& e : m_edges) {
            if (e.fromNode == n) {
                if (--inDegree[e.toNode] == 0) {
                    q.push(e.toNode);
                }
            }
        }
    }

    return visited != static_cast<int>(m_nodes.size());
}

bool LODBakingGraph::Compile() {
    m_compiled = false;
    m_executionOrder.clear();

    if (HasCycle()) return false;

    // Topological sort using Kahn's algorithm
    std::unordered_map<uint32_t, int> inDegree;
    for (auto& [id, _] : m_nodes) {
        inDegree[id] = 0;
    }
    for (auto& e : m_edges) {
        inDegree[e.toNode]++;
    }

    std::queue<uint32_t> q;
    for (auto& [id, deg] : inDegree) {
        if (deg == 0) q.push(id);
    }

    while (!q.empty()) {
        uint32_t n = q.front();
        q.pop();
        m_executionOrder.push_back(n);
        for (auto& e : m_edges) {
            if (e.fromNode == n) {
                if (--inDegree[e.toNode] == 0) {
                    q.push(e.toNode);
                }
            }
        }
    }

    m_compiled = (m_executionOrder.size() == m_nodes.size());
    return m_compiled;
}

void LODBakingGraph::ExecuteNode(const LODNode& node) {
    // Collect input meshes from upstream edges (port 0 = primary)
    MeshData input0;
    for (auto& e : m_edges) {
        if (e.toNode == node.id) {
            uint64_t key = (static_cast<uint64_t>(e.fromNode) << 16) | e.fromPort;
            auto outIt = m_meshOutputs.find(key);
            if (outIt != m_meshOutputs.end()) {
                if (e.toPort == 0) input0 = outIt->second;
            }
        }
    }

    MeshData result;

    switch (node.type) {
        case LODNodeType::MeshInput: {
            std::string shape = node.GetProperty("shape", "cube");
            float size = SafeStof(node.GetProperty("size"), 1.0f);
            int segments = SafeStoi(node.GetProperty("segments"), 8);

            if (shape == "sphere") result = GenerateSphere(size, segments);
            else result = GenerateCube(size);
            break;
        }
        case LODNodeType::Decimate: {
            float factor = SafeStof(node.GetProperty("factor"), 0.5f);
            result = DecimateMesh(input0, factor);
            break;
        }
        case LODNodeType::MergeVertices: {
            float threshold = SafeStof(node.GetProperty("threshold"), 0.001f);
            result = MergeNearbyVertices(input0, threshold);
            break;
        }
        case LODNodeType::BakeNormals: {
            result = RecomputeNormals(input0);
            break;
        }
        case LODNodeType::Output: {
            int levels = SafeStoi(node.GetProperty("levels"), 3);
            if (levels < 1) levels = 1;
            m_lodOutput = GenerateLODChain(input0, static_cast<uint32_t>(levels));
            result = input0;
            break;
        }
    }

    uint64_t key = (static_cast<uint64_t>(node.id) << 16) | 0;
    m_meshOutputs[key] = std::move(result);
}

bool LODBakingGraph::Execute() {
    if (!m_compiled) return false;

    m_meshOutputs.clear();
    m_lodOutput = LODChain();

    for (uint32_t id : m_executionOrder) {
        auto it = m_nodes.find(id);
        if (it == m_nodes.end()) return false;
        ExecuteNode(it->second);
    }

    return true;
}

const LODChain* LODBakingGraph::GetOutput() const {
    // Find the Output node and return the LOD chain
    for (auto& [id, node] : m_nodes) {
        if (node.type == LODNodeType::Output) {
            if (m_lodOutput.IsValid()) return &m_lodOutput;
        }
    }
    return nullptr;
}

size_t LODBakingGraph::NodeCount() const {
    return m_nodes.size();
}

bool LODBakingGraph::IsCompiled() const {
    return m_compiled;
}

}
