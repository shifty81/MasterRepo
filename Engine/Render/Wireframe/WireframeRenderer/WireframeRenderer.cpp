#include "Engine/Render/Wireframe/WireframeRenderer/WireframeRenderer.h"
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace Engine {

struct MeshEntry {
    uint32_t id;
    std::vector<float>    verts; // pos[3] per vertex
    std::vector<uint32_t> indices;
    float transform[16];
};

struct WireframeRenderer::Impl {
    std::vector<MeshEntry> meshes;
    std::vector<WireEdge>  lines;
    float colour[4]{0,1,0,1};
    float depthBias{0.001f};
    float thickness{1.f};
    bool  enabled{true};
    bool  dirty{true};

    MeshEntry* Find(uint32_t id){ for(auto& m:meshes) if(m.id==id) return &m; return nullptr; }
};

static void IdentityMat(float m[16]){ memset(m,0,64); m[0]=m[5]=m[10]=m[15]=1.f; }
static void TransformVec3(const float mat[16], const float v[3], float out[3]){
    out[0]=mat[0]*v[0]+mat[4]*v[1]+mat[8]*v[2]+mat[12];
    out[1]=mat[1]*v[0]+mat[5]*v[1]+mat[9]*v[2]+mat[13];
    out[2]=mat[2]*v[0]+mat[6]*v[1]+mat[10]*v[2]+mat[14];
}

WireframeRenderer::WireframeRenderer() : m_impl(new Impl){}
WireframeRenderer::~WireframeRenderer(){ Shutdown(); delete m_impl; }
void WireframeRenderer::Init()     {}
void WireframeRenderer::Shutdown() { m_impl->meshes.clear(); m_impl->lines.clear(); }
void WireframeRenderer::Clear()    { m_impl->meshes.clear(); m_impl->lines.clear(); m_impl->dirty=true; }

void WireframeRenderer::AddMesh(uint32_t id, const float* v, uint32_t vc,
                                  const uint32_t* idx, uint32_t ic, const float tf[16]){
    MeshEntry e; e.id=id;
    e.verts.assign(v,v+vc*3);
    e.indices.assign(idx,idx+ic);
    if(tf) memcpy(e.transform,tf,64);
    else   IdentityMat(e.transform);
    m_impl->meshes.push_back(e);
    m_impl->dirty=true;
}
void WireframeRenderer::RemoveMesh(uint32_t id){
    auto& v=m_impl->meshes;
    v.erase(std::remove_if(v.begin(),v.end(),[id](auto& m){return m.id==id;}),v.end());
    m_impl->dirty=true;
}
bool WireframeRenderer::HasMesh(uint32_t id) const { return m_impl->Find(id)!=nullptr; }
void WireframeRenderer::UpdateTransform(uint32_t id, const float tf[16]){ auto* m=m_impl->Find(id); if(m){memcpy(m->transform,tf,64); m_impl->dirty=true;} }

std::vector<WireEdge> WireframeRenderer::ExtractEdges(const float* v, uint32_t /*vc*/,
                                                        const uint32_t* idx, uint32_t ic){
    std::vector<WireEdge> edges;
    for(uint32_t i=0;i+2<ic;i+=3){
        for(int e=0;e<3;e++){
            uint32_t a=idx[i+e], b=idx[i+(e+1)%3];
            WireEdge we;
            for(int k=0;k<3;k++){we.a[k]=v[a*3+k]; we.b[k]=v[b*3+k];}
            edges.push_back(we);
        }
    }
    return edges;
}

void WireframeRenderer::Bake(){
    if(!m_impl->dirty) return;
    m_impl->lines.clear();
    for(auto& m:m_impl->meshes){
        auto raw=ExtractEdges(m.verts.data(),(uint32_t)m.verts.size()/3,m.indices.data(),(uint32_t)m.indices.size());
        for(auto& e:raw){
            WireEdge we;
            TransformVec3(m.transform,e.a,we.a);
            TransformVec3(m.transform,e.b,we.b);
            m_impl->lines.push_back(we);
        }
    }
    m_impl->dirty=false;
}

const std::vector<WireEdge>& WireframeRenderer::GetLines() const { return m_impl->lines; }
void WireframeRenderer::SetColour(float r,float g,float b,float a){ m_impl->colour[0]=r; m_impl->colour[1]=g; m_impl->colour[2]=b; m_impl->colour[3]=a; }
void WireframeRenderer::GetColour(float& r,float& g,float& b,float& a) const { r=m_impl->colour[0]; g=m_impl->colour[1]; b=m_impl->colour[2]; a=m_impl->colour[3]; }
void WireframeRenderer::SetDepthBias(float b){ m_impl->depthBias=b; }
void WireframeRenderer::SetThickness(float t){ m_impl->thickness=t; }
void WireframeRenderer::SetEnabled(bool e){ m_impl->enabled=e; }
bool WireframeRenderer::IsEnabled() const  { return m_impl->enabled; }

} // namespace Engine
