#include "Builder/Blueprint/BlueprintCompiler.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Builder {

// ── Lua code generator ────────────────────────────────────────────────────────

static std::string NodeToLua(const BPNode& n,
    const std::unordered_map<uint32_t,BPNode>& nodeMap,
    const std::vector<BPWire>& wires) {
    (void)nodeMap; (void)wires;
    std::ostringstream s;
    switch(n.type){
        case BPNodeType::PrintString:
            s<<"  print(\"" << n.label << "\")\n"; break;
        case BPNodeType::CallFunction:
            s<<"  " << n.functionName << "()\n"; break;
        case BPNodeType::CallEngineAPI:
            s<<"  Engine." << n.functionName << "()\n"; break;
        case BPNodeType::EventBeginPlay:
            s<<"function OnBeginPlay()\n"; break;
        case BPNodeType::EventTick:
            s<<"function OnTick(dt)\n"; break;
        case BPNodeType::Branch: {
            std::string cond="condition";
            for(auto& p:n.inputs) if(p.name=="Condition") cond=p.defaultValue;
            s<<"  if "<<cond<<" then\n"; break;
        }
        case BPNodeType::SetVariable:
            s<<"  "<<n.variableName<<"="<<(n.inputs.empty()?"nil":n.inputs[0].defaultValue)<<"\n"; break;
        case BPNodeType::GetVariable:
            s<<"  local _"<<n.variableName<<"="<<n.variableName<<"\n"; break;
        default:
            s<<"  -- node "<<n.id<<"\n"; break;
    }
    return s.str();
}

// ── C++ code generator ────────────────────────────────────────────────────────

static std::string NodeToCPP(const BPNode& n) {
    std::ostringstream s;
    switch(n.type){
        case BPNodeType::PrintString:
            s<<"    Logger::Info(\""<<n.label<<"\");\n"; break;
        case BPNodeType::CallFunction:
            s<<"    "<<n.functionName<<"();\n"; break;
        case BPNodeType::EventBeginPlay:
            s<<"void BeginPlay() {\n"; break;
        case BPNodeType::EventTick:
            s<<"void Tick(float dt) {\n"; break;
        default:
            s<<"    // node "<<n.id<<"\n"; break;
    }
    return s.str();
}

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct BlueprintCompiler::Impl {
    std::unordered_map<std::string,std::string> apiRegistry;
    std::function<void(const BlueprintCompileResult&)> onCompile;
};

BlueprintCompiler::BlueprintCompiler() : m_impl(new Impl()) {}
BlueprintCompiler::~BlueprintCompiler() { delete m_impl; }
void BlueprintCompiler::Init()     {}
void BlueprintCompiler::Shutdown() {}

BlueprintCompileResult BlueprintCompiler::Compile(const BlueprintGraph& graph,
                                                   BlueprintTarget target) {
    BlueprintCompileResult r;
    auto errs=Validate(graph);
    if(!errs.empty()){ r.errorMessage=errs[0]; if(m_impl->onCompile) m_impl->onCompile(r); return r; }

    std::unordered_map<uint32_t,BPNode> nodeMap;
    for(auto& n:graph.nodes) nodeMap[n.id]=n;

    // Topological order via in-degree (execution flow wires)
    std::unordered_map<uint32_t,int> indeg;
    for(auto& n:graph.nodes) indeg[n.id]=0;
    for(auto& w:graph.wires){
        auto fn=nodeMap.find(w.fromNode); auto tn=nodeMap.find(w.toNode);
        if(fn!=nodeMap.end()&&tn!=nodeMap.end()&&
           w.fromPin<fn->second.outputs.size()&&fn->second.outputs[w.fromPin].isExec)
            ++indeg[w.toNode];
    }
    std::vector<uint32_t> order;
    std::vector<uint32_t> queue;
    for(auto&[id,d]:indeg) if(d==0) queue.push_back(id);
    while(!queue.empty()){ uint32_t id=queue.back(); queue.pop_back(); order.push_back(id);
        for(auto& w:graph.wires){ if(w.fromNode==id&&--indeg[w.toNode]==0) queue.push_back(w.toNode); } }

    std::ostringstream src;
    if(target==BlueprintTarget::Lua){
        src<<"-- Blueprint: "<<graph.name<<"\n";
        src<<"-- extends: "<<graph.targetClass<<"\n\n";
        for(uint32_t id:order){
            auto it=nodeMap.find(id); if(it==nodeMap.end()) continue;
            src<<NodeToLua(it->second,nodeMap,graph.wires);
        }
        // Close any open function blocks
        src<<"end\n";
    } else {
        src<<"// Blueprint: "<<graph.name<<"\n";
        src<<"// extends: "<<graph.targetClass<<"\n\n";
        src<<"#include \"Engine/Core/Logger.h\"\n\n";
        src<<"class "<<graph.name<<" : public "<<graph.targetClass<<" {\npublic:\n";
        for(uint32_t id:order){
            auto it=nodeMap.find(id); if(it==nodeMap.end()) continue;
            src<<NodeToCPP(it->second);
        }
        src<<"}\n};\n";
    }
    r.source    = src.str();
    r.succeeded = true;
    if(m_impl->onCompile) m_impl->onCompile(r);
    return r;
}

std::vector<std::string> BlueprintCompiler::Validate(const BlueprintGraph& graph) const {
    std::vector<std::string> errs;
    if(graph.nodes.empty()) errs.push_back("Graph has no nodes");
    // Check for duplicate node IDs
    std::unordered_set<uint32_t> seen;
    for(auto& n:graph.nodes){ if(!seen.insert(n.id).second) errs.push_back("Duplicate node ID "+std::to_string(n.id)); }
    return errs;
}

bool BlueprintCompiler::SaveGraph(const BlueprintGraph& graph, const std::string& path) {
    std::ofstream f(path); if(!f.is_open()) return false;
    f<<"{\"name\":\""<<graph.name<<"\",\"nodeCount\":"<<graph.nodes.size()<<"}\n";
    return true;
}

BlueprintGraph BlueprintCompiler::LoadGraph(const std::string& path) {
    BlueprintGraph g; std::ifstream f(path); (void)f; return g;
}

void BlueprintCompiler::RegisterEngineAPI(const std::string& name,
                                           const std::string& sig) {
    m_impl->apiRegistry[name]=sig;
}

bool BlueprintCompiler::IsKnownAPI(const std::string& name) const {
    return m_impl->apiRegistry.count(name)>0;
}

void BlueprintCompiler::OnCompileComplete(
    std::function<void(const BlueprintCompileResult&)> cb) {
    m_impl->onCompile=std::move(cb);
}

} // namespace Builder
