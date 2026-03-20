#include "PCG/World/WorldNode.h"
#include <algorithm>
#include <functional>

namespace PCG {

WorldNodeID WorldGraph::AddNode(std::unique_ptr<IWorldNode> node) {
    WorldNodeID id = m_nextID++;
    m_nodes[id] = std::move(node);
    m_compiled = false;
    return id;
}

void WorldGraph::RemoveNode(WorldNodeID id) {
    m_nodes.erase(id);
    m_edges.erase(std::remove_if(m_edges.begin(), m_edges.end(),
        [id](const WorldEdge& e){ return e.fromNode==id||e.toNode==id; }),
        m_edges.end());
    m_compiled = false;
}

void WorldGraph::AddEdge(const WorldEdge& edge) { m_edges.push_back(edge); m_compiled=false; }

void WorldGraph::RemoveEdge(const WorldEdge& edge) {
    m_edges.erase(std::remove_if(m_edges.begin(), m_edges.end(),
        [&](const WorldEdge& e){
            return e.fromNode==edge.fromNode&&e.fromPort==edge.fromPort&&
                   e.toNode==edge.toNode&&e.toPort==edge.toPort;
        }), m_edges.end());
}

bool WorldGraph::HasCycle() const {
    std::unordered_map<WorldNodeID,int> color;
    for (auto& [id,_]:m_nodes) color[id]=0;
    std::function<bool(WorldNodeID)> dfs=[&](WorldNodeID n)->bool{
        color[n]=1;
        for (auto& e:m_edges) if(e.fromNode==n){
            if(color[e.toNode]==1) return true;
            if(color[e.toNode]==0&&dfs(e.toNode)) return true;
        }
        color[n]=2; return false;
    };
    for (auto& [id,_]:m_nodes) if(color[id]==0&&dfs(id)) return true;
    return false;
}

bool WorldGraph::ValidateEdgeTypes() const { return true; }

bool WorldGraph::Compile() {
    if (HasCycle()) return false;
    m_executionOrder.clear();
    std::unordered_map<WorldNodeID,int> inDeg;
    for (auto& [id,_]:m_nodes) inDeg[id]=0;
    for (auto& e:m_edges) inDeg[e.toNode]++;
    std::vector<WorldNodeID> q;
    for (auto& [id,d]:inDeg) if(d==0) q.push_back(id);
    while (!q.empty()) {
        WorldNodeID n=q.back(); q.pop_back();
        m_executionOrder.push_back(n);
        for (auto& e:m_edges) if(e.fromNode==n&&--inDeg[e.toNode]==0) q.push_back(e.toNode);
    }
    m_compiled=(m_executionOrder.size()==m_nodes.size());
    return m_compiled;
}

bool WorldGraph::Execute(const WorldGenContext& ctx) {
    if (!m_compiled) return false;
    m_outputs.clear();
    for (WorldNodeID nid:m_executionOrder) {
        auto it=m_nodes.find(nid); if(it==m_nodes.end()) continue;
        auto inPorts=it->second->Inputs();
        std::vector<WorldValue> inputs(inPorts.size());
        for (size_t pi=0;pi<inPorts.size();++pi)
            for (auto& e:m_edges) if(e.toNode==nid&&e.toPort==(WorldPortID)pi) {
                uint64_t key=((uint64_t)e.fromNode<<16)|e.fromPort;
                auto oit=m_outputs.find(key); if(oit!=m_outputs.end()) inputs[pi]=oit->second;
            }
        auto outPorts=it->second->Outputs();
        std::vector<WorldValue> outs(outPorts.size());
        it->second->Evaluate(ctx,inputs,outs);
        for (size_t pi=0;pi<outs.size();++pi)
            m_outputs[((uint64_t)nid<<16)|(WorldPortID)pi]=outs[pi];
    }
    return true;
}

const WorldValue* WorldGraph::GetOutput(WorldNodeID node,WorldPortID port) const {
    auto it=m_outputs.find(((uint64_t)node<<16)|port);
    return it!=m_outputs.end()?&it->second:nullptr;
}

size_t WorldGraph::NodeCount()  const { return m_nodes.size(); }
bool   WorldGraph::IsCompiled() const { return m_compiled; }

} // namespace PCG
