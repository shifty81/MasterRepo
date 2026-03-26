#include "Engine/Shader/ShaderGraph/ShaderGraph.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Engine {

// ── Pin defaults by node type ─────────────────────────────────────────────────

static void BuildPins(ShaderNode& n) {
    auto in  = [&](const char* name, PinType t){ ShaderPin p; p.name=name; p.type=t; p.nodeId=n.id; p.pinIndex=(uint32_t)n.inputs.size(); n.inputs.push_back(p); };
    auto out = [&](const char* name, PinType t){ ShaderPin p; p.name=name; p.type=t; p.isOutput=true; p.nodeId=n.id; p.pinIndex=(uint32_t)n.outputs.size(); n.outputs.push_back(p); };
    switch (n.type) {
        case ShaderNodeType::Float:         out("Value",PinType::Float1); break;
        case ShaderNodeType::Vec2:          out("Value",PinType::Float2); break;
        case ShaderNodeType::Vec3:          out("Value",PinType::Float3); break;
        case ShaderNodeType::Vec4:          out("Value",PinType::Float4); break;
        case ShaderNodeType::Colour:        out("Colour",PinType::Float4); break;
        case ShaderNodeType::TextureSample: in("UV",PinType::Float2); out("RGBA",PinType::Float4); out("R",PinType::Float1); out("G",PinType::Float1); out("B",PinType::Float1); out("A",PinType::Float1); break;
        case ShaderNodeType::UV:            out("UV",PinType::Float2); break;
        case ShaderNodeType::Time:          out("Time",PinType::Float1); break;
        case ShaderNodeType::Add:
        case ShaderNodeType::Subtract:
        case ShaderNodeType::Multiply:
        case ShaderNodeType::Divide:        in("A",PinType::Float4); in("B",PinType::Float4); out("Result",PinType::Float4); break;
        case ShaderNodeType::Lerp:          in("A",PinType::Float4); in("B",PinType::Float4); in("T",PinType::Float1); out("Result",PinType::Float4); break;
        case ShaderNodeType::Clamp:         in("Value",PinType::Float1); in("Min",PinType::Float1); in("Max",PinType::Float1); out("Result",PinType::Float1); break;
        case ShaderNodeType::Normalize:     in("Vec",PinType::Float3); out("Result",PinType::Float3); break;
        case ShaderNodeType::Fresnel:       in("Normal",PinType::Float3); in("Power",PinType::Float1); out("Factor",PinType::Float1); break;
        case ShaderNodeType::SurfaceOutput: in("Albedo",PinType::Float3); in("Roughness",PinType::Float1); in("Metallic",PinType::Float1); in("Emissive",PinType::Float3); in("Normal",PinType::Float3); in("Alpha",PinType::Float1); break;
        default: in("In",PinType::Float4); out("Out",PinType::Float4); break;
    }
}

// ── Code generation helpers ───────────────────────────────────────────────────

static const char* PinTypeName(PinType t) {
    switch(t) { case PinType::Float1: return "float"; case PinType::Float2: return "vec2"; case PinType::Float3: return "vec3"; case PinType::Sampler2D: return "sampler2D"; default: return "vec4"; }
}

static std::string NodeVarName(uint32_t id, uint32_t pin) {
    return "n" + std::to_string(id) + "_p" + std::to_string(pin);
}

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct ShaderGraph::Impl {
    std::unordered_map<uint32_t, ShaderNode> nodes;
    std::vector<ShaderEdge> edges;
    uint32_t nextId{1};
    std::function<void(uint32_t)>       onNodeAdded;
    std::function<void(const ShaderEdge&)> onEdgeAdded;
};

ShaderGraph::ShaderGraph() : m_impl(new Impl()) {}
ShaderGraph::~ShaderGraph() { delete m_impl; }
void ShaderGraph::Init()     {}
void ShaderGraph::Shutdown() { Clear(); }

uint32_t ShaderGraph::AddNode(ShaderNodeType type, const std::string& label) {
    uint32_t id = m_impl->nextId++;
    ShaderNode& n = m_impl->nodes[id];
    n.id    = id;
    n.type  = type;
    n.label = label.empty() ? "Node" : label;
    BuildPins(n);
    if (m_impl->onNodeAdded) m_impl->onNodeAdded(id);
    return id;
}

void ShaderGraph::RemoveNode(uint32_t id) {
    m_impl->nodes.erase(id);
    auto& e = m_impl->edges;
    e.erase(std::remove_if(e.begin(),e.end(),[id](const ShaderEdge& ed){
        return ed.fromNode==id||ed.toNode==id; }), e.end());
}

bool ShaderGraph::HasNode(uint32_t id) const { return m_impl->nodes.count(id)>0; }

ShaderNode ShaderGraph::GetNode(uint32_t id) const {
    auto it = m_impl->nodes.find(id);
    return it!=m_impl->nodes.end() ? it->second : ShaderNode{};
}

void ShaderGraph::SetNodePosition(uint32_t id, float x, float y) {
    auto it = m_impl->nodes.find(id); if(it!=m_impl->nodes.end()) { it->second.posX=x; it->second.posY=y; }
}
void ShaderGraph::SetNodeValue(uint32_t id, float v) {
    auto it = m_impl->nodes.find(id); if(it!=m_impl->nodes.end()) it->second.floatValue=v;
}
void ShaderGraph::SetNodeVecValue(uint32_t id, const float v[4]) {
    auto it = m_impl->nodes.find(id); if(it!=m_impl->nodes.end()) for(int i=0;i<4;++i) it->second.vecValue[i]=v[i];
}
void ShaderGraph::SetNodeTexture(uint32_t id, const std::string& p) {
    auto it = m_impl->nodes.find(id); if(it!=m_impl->nodes.end()) it->second.textureAsset=p;
}

std::vector<ShaderNode> ShaderGraph::AllNodes() const {
    std::vector<ShaderNode> out; for(auto&[k,v]:m_impl->nodes) out.push_back(v); return out;
}

bool ShaderGraph::Connect(uint32_t fn, uint32_t fp, uint32_t tn, uint32_t tp) {
    if (!HasNode(fn)||!HasNode(tn)) return false;
    ShaderEdge e{fn,fp,tn,tp};
    m_impl->edges.push_back(e);
    if (m_impl->onEdgeAdded) m_impl->onEdgeAdded(e);
    return true;
}

void ShaderGraph::Disconnect(uint32_t fn,uint32_t fp,uint32_t tn,uint32_t tp) {
    auto& e=m_impl->edges;
    e.erase(std::remove_if(e.begin(),e.end(),[&](const ShaderEdge& ed){
        return ed.fromNode==fn&&ed.fromPin==fp&&ed.toNode==tn&&ed.toPin==tp;}),e.end());
}

bool ShaderGraph::IsConnected(uint32_t fn,uint32_t fp,uint32_t tn,uint32_t tp) const {
    for(auto& e:m_impl->edges) if(e.fromNode==fn&&e.fromPin==fp&&e.toNode==tn&&e.toPin==tp) return true;
    return false;
}

std::vector<ShaderEdge> ShaderGraph::AllEdges() const { return m_impl->edges; }

ShaderCompileResult ShaderGraph::Compile(ShaderTarget target) const {
    ShaderCompileResult r;
    // Topological sort
    std::vector<uint32_t> order;
    {
        std::unordered_map<uint32_t,int> indegree;
        for(auto&[id,_]:m_impl->nodes) indegree[id]=0;
        for(auto& e:m_impl->edges) ++indegree[e.toNode];
        std::vector<uint32_t> q;
        for(auto&[id,d]:indegree) if(d==0) q.push_back(id);
        while(!q.empty()){ uint32_t n=q.back(); q.pop_back(); order.push_back(n); for(auto&e:m_impl->edges) if(e.fromNode==n&&--indegree[e.toNode]==0) q.push_back(e.toNode); }
    }
    if(order.size()!=m_impl->nodes.size()){ r.errorMessage="Cycle detected in shader graph"; return r; }

    const char* version = (target==ShaderTarget::GLSL) ? "#version 330 core\n" : "// HLSL\n";
    std::ostringstream frag;
    frag << version << "out vec4 FragColor;\n";
    // Uniforms for texture nodes
    for(auto&[id,n]:m_impl->nodes)
        if(n.type==ShaderNodeType::TextureSample)
            frag<<"uniform sampler2D u_tex"<<id<<";\n";
    frag<<"void main(){\n";
    for(uint32_t nid:order){
        auto& n=m_impl->nodes.at(nid);
        switch(n.type){
            case ShaderNodeType::Float:   frag<<"  float "<<NodeVarName(nid,0)<<"="<<n.floatValue<<";\n"; break;
            case ShaderNodeType::Vec3:    frag<<"  vec3 "<<NodeVarName(nid,0)<<"=vec3("<<n.vecValue[0]<<","<<n.vecValue[1]<<","<<n.vecValue[2]<<");\n"; break;
            case ShaderNodeType::Add:     frag<<"  vec4 "<<NodeVarName(nid,0)<<"="<<NodeVarName(nid-1,0)<<"+"<<NodeVarName(nid-1,1)<<";\n"; break;
            case ShaderNodeType::Multiply:frag<<"  vec4 "<<NodeVarName(nid,0)<<"="<<NodeVarName(nid-1,0)<<"*"<<NodeVarName(nid-1,1)<<";\n"; break;
            case ShaderNodeType::TextureSample: frag<<"  vec4 "<<NodeVarName(nid,0)<<"=texture(u_tex"<<nid<<",vec2(0));\n"; break;
            case ShaderNodeType::SurfaceOutput: frag<<"  FragColor=vec4(1.0);\n"; break;
            default: break;
        }
    }
    frag<<"}\n";
    r.fragmentSource = frag.str();
    r.vertexSource   = std::string(version) +
        "layout(location=0) in vec3 aPos;\nlayout(location=1) in vec2 aUV;\nout vec2 vUV;\n"
        "uniform mat4 uMVP;\nvoid main(){vUV=aUV;gl_Position=uMVP*vec4(aPos,1.0);}\n";
    r.succeeded = true;
    return r;
}

bool ShaderGraph::SaveToFile(const std::string& path) const {
    std::ofstream f(path); if(!f.is_open()) return false;
    f<<"{\"nodes\":"<<m_impl->nodes.size()<<",\"edges\":"<<m_impl->edges.size()<<"}\n";
    return true;
}
bool ShaderGraph::LoadFromFile(const std::string& path) {
    std::ifstream f(path); return f.is_open();
}
std::string ShaderGraph::ToJSON() const {
    std::ostringstream s; s<<"{\"nodeCount\":"<<m_impl->nodes.size()<<"}"; return s.str();
}
bool ShaderGraph::FromJSON(const std::string&) { return true; }
void ShaderGraph::Clear() { m_impl->nodes.clear(); m_impl->edges.clear(); }

bool ShaderGraph::Validate(std::vector<std::string>& errors) const {
    errors.clear();
    bool hasSurface = false;
    for(auto&[id,n]:m_impl->nodes) if(n.type==ShaderNodeType::SurfaceOutput){ hasSurface=true; break; }
    if(!hasSurface) errors.push_back("No SurfaceOutput node found");
    return errors.empty();
}

void ShaderGraph::OnNodeAdded(std::function<void(uint32_t)> cb) { m_impl->onNodeAdded=std::move(cb); }
void ShaderGraph::OnEdgeAdded(std::function<void(const ShaderEdge&)> cb) { m_impl->onEdgeAdded=std::move(cb); }

} // namespace Engine
