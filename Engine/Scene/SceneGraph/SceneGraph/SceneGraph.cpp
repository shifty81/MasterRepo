#include "Engine/Scene/SceneGraph/SceneGraph/SceneGraph.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

static SGMat4 TRSToMat4(const SGTransform& t) {
    // Build TRS matrix from pos/rot(quat)/scale
    float qx=t.rot.x, qy=t.rot.y, qz=t.rot.z, qw=t.rot.w;
    float sx=t.scale.x, sy=t.scale.y, sz=t.scale.z;
    SGMat4 m{};
    m.m[0]=(1-2*(qy*qy+qz*qz))*sx;
    m.m[1]=(2*(qx*qy+qz*qw))*sx;
    m.m[2]=(2*(qx*qz-qy*qw))*sx;
    m.m[3]=0;
    m.m[4]=(2*(qx*qy-qz*qw))*sy;
    m.m[5]=(1-2*(qx*qx+qz*qz))*sy;
    m.m[6]=(2*(qy*qz+qx*qw))*sy;
    m.m[7]=0;
    m.m[8]=(2*(qx*qz+qy*qw))*sz;
    m.m[9]=(2*(qy*qz-qx*qw))*sz;
    m.m[10]=(1-2*(qx*qx+qy*qy))*sz;
    m.m[11]=0;
    m.m[12]=t.pos.x; m.m[13]=t.pos.y; m.m[14]=t.pos.z; m.m[15]=1;
    return m;
}

static SGMat4 MatMul(const SGMat4& A, const SGMat4& B) {
    SGMat4 C{};
    for(int r=0;r<4;r++) for(int c=0;c<4;c++){
        float s=0; for(int k=0;k<4;k++) s+=A.m[r+k*4]*B.m[k+c*4];
        C.m[r+c*4]=s;
    }
    return C;
}

static SGTransform Mat4ToTransform(const SGMat4& m) {
    SGTransform t;
    t.pos={m.m[12],m.m[13],m.m[14]};
    // Extract scale
    float sx=std::sqrt(m.m[0]*m.m[0]+m.m[1]*m.m[1]+m.m[2]*m.m[2]);
    float sy=std::sqrt(m.m[4]*m.m[4]+m.m[5]*m.m[5]+m.m[6]*m.m[6]);
    float sz=std::sqrt(m.m[8]*m.m[8]+m.m[9]*m.m[9]+m.m[10]*m.m[10]);
    t.scale={sx,sy,sz};
    // Extract rotation (from normalised matrix)
    float r00=m.m[0]/sx,r11=m.m[5]/sy,r22=m.m[10]/sz;
    float trace=r00+r11+r22;
    if(trace>0){
        float s=0.5f/std::sqrt(trace+1.f);
        t.rot.w=0.25f/s;
        t.rot.x=(m.m[6]/sz-m.m[9]/sy)*s;
        t.rot.y=(m.m[8]/sx-m.m[2]/sz)*s;
        t.rot.z=(m.m[1]/sy-m.m[4]/sx)*s;
    } else {
        t.rot={0,0,0,1};
    }
    return t;
}

struct SGNode {
    uint32_t   id{0};
    std::string name;
    uint32_t   parent{0};
    std::vector<uint32_t> children;
    SGTransform localTransform;
    SGMat4      worldMatrix{};
    bool        dirty{true};
    std::unordered_map<uint32_t,void*> components;
};

struct SceneGraph::Impl {
    std::unordered_map<uint32_t, SGNode> nodes;
    uint32_t nextId{1};

    SGNode* Find(uint32_t id){ auto it=nodes.find(id); return it!=nodes.end()?&it->second:nullptr; }
    const SGNode* Find(uint32_t id) const { auto it=nodes.find(id); return it!=nodes.end()?&it->second:nullptr; }

    void MarkDirty(uint32_t id){
        auto* n=Find(id); if(!n||n->dirty) return;
        n->dirty=true;
        for(auto c:n->children) MarkDirty(c);
    }

    void UpdateNode(uint32_t id, const SGMat4& parentWorld){
        auto* n=Find(id); if(!n) return;
        if(n->dirty){
            SGMat4 local=TRSToMat4(n->localTransform);
            n->worldMatrix=MatMul(parentWorld,local);
            n->dirty=false;
        }
        for(auto c:n->children) UpdateNode(c,n->worldMatrix);
    }

    SGMat4 Identity(){ SGMat4 m{}; m.m[0]=m.m[5]=m.m[10]=m.m[15]=1; return m; }

    void CollectDescendants(uint32_t id, std::vector<uint32_t>& out) const {
        auto* n=Find(id); if(!n) return;
        for(auto c:n->children){ out.push_back(c); CollectDescendants(c,out); }
    }

    void DestroySubtree(uint32_t id){
        auto* n=Find(id); if(!n) return;
        std::vector<uint32_t> ch=n->children;
        for(auto c:ch) DestroySubtree(c);
        // Remove from parent's child list
        if(n->parent){
            auto* par=Find(n->parent);
            if(par) par->children.erase(
                std::remove(par->children.begin(),par->children.end(),id),
                par->children.end());
        }
        nodes.erase(id);
    }
};

SceneGraph::SceneGraph(): m_impl(new Impl){}
SceneGraph::~SceneGraph(){ Shutdown(); delete m_impl; }
void SceneGraph::Init(){}
void SceneGraph::Shutdown(){ m_impl->nodes.clear(); m_impl->nextId=1; }
void SceneGraph::Reset(){ Shutdown(); }

uint32_t SceneGraph::CreateNode(const std::string& name, uint32_t parentId){
    SGNode n; n.id=m_impl->nextId++; n.name=name; n.parent=parentId;
    n.worldMatrix=m_impl->Identity(); n.dirty=true;
    m_impl->nodes[n.id]=n;
    if(parentId){
        auto* par=m_impl->Find(parentId);
        if(par) par->children.push_back(n.id);
    }
    return n.id;
}

void SceneGraph::DestroyNode(uint32_t id){ m_impl->DestroySubtree(id); }
bool SceneGraph::Exists(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void SceneGraph::SetParent(uint32_t childId, uint32_t parentId){
    auto* child=m_impl->Find(childId); if(!child) return;
    // Validate parentId (0 = root is always valid)
    if(parentId!=0 && !m_impl->Find(parentId)) return;
    // Remove from old parent
    if(child->parent){
        auto* old=m_impl->Find(child->parent);
        if(old) old->children.erase(
            std::remove(old->children.begin(),old->children.end(),childId),
            old->children.end());
    }
    child->parent=parentId;
    if(parentId){
        auto* par=m_impl->Find(parentId);
        if(par) par->children.push_back(childId);
    }
    m_impl->MarkDirty(childId);
}

uint32_t SceneGraph::GetParent(uint32_t id) const {
    auto* n=m_impl->Find(id); return n?n->parent:0;
}
std::vector<uint32_t> SceneGraph::GetChildren(uint32_t id) const {
    auto* n=m_impl->Find(id); return n?n->children:std::vector<uint32_t>{};
}
std::vector<uint32_t> SceneGraph::GetDescendants(uint32_t id) const {
    std::vector<uint32_t> out; m_impl->CollectDescendants(id,out); return out;
}

void SceneGraph::SetLocalTransform(uint32_t id, const SGTransform& t){
    auto* n=m_impl->Find(id); if(!n) return;
    n->localTransform=t; m_impl->MarkDirty(id);
}
SGTransform SceneGraph::GetLocalTransform(uint32_t id) const {
    auto* n=m_impl->Find(id); return n?n->localTransform:SGTransform{};
}
SGTransform SceneGraph::GetWorldTransform(uint32_t id) const {
    auto* n=m_impl->Find(id); return n?Mat4ToTransform(n->worldMatrix):SGTransform{};
}
SGMat4 SceneGraph::GetWorldMatrix(uint32_t id) const {
    auto* n=m_impl->Find(id); return n?n->worldMatrix:m_impl->Identity();
}

void SceneGraph::Update(){
    SGMat4 identity=m_impl->Identity();
    // Update root nodes (parentId==0)
    for(auto& [id,n]:m_impl->nodes)
        if(n.parent==0) m_impl->UpdateNode(id,identity);
}

void SceneGraph::AttachComponent(uint32_t nodeId, uint32_t typeId, void* ptr){
    auto* n=m_impl->Find(nodeId); if(n) n->components[typeId]=ptr;
}
void* SceneGraph::GetComponent(uint32_t nodeId, uint32_t typeId) const {
    auto* n=m_impl->Find(nodeId); if(!n) return nullptr;
    auto it=n->components.find(typeId); return (it!=n->components.end())?it->second:nullptr;
}

uint32_t SceneGraph::FindByName(const std::string& name) const {
    for(auto& [id,n]:m_impl->nodes) if(n.name==name) return id;
    return 0;
}
std::string SceneGraph::GetName(uint32_t id) const {
    auto* n=m_impl->Find(id); return n?n->name:"";
}

void SceneGraph::Traverse(std::function<void(uint32_t,uint32_t)> visitor) const {
    std::function<void(uint32_t,uint32_t)> dfs=[&](uint32_t id, uint32_t depth){
        visitor(id,depth);
        auto* n=m_impl->Find(id); if(!n) return;
        for(auto c:n->children) dfs(c,depth+1);
    };
    for(auto& [id,n]:m_impl->nodes) if(n.parent==0) dfs(id,0);
}

uint32_t SceneGraph::GetNodeCount() const { return (uint32_t)m_impl->nodes.size(); }

} // namespace Engine
