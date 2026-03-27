#include "Engine/Render/ProceduralMesh/ProceduralMeshSystem/ProceduralMeshSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

static constexpr float kPI = 3.14159265f;

struct Vertex { float x,y,z,nx,ny,nz,u,v; };

struct MeshData {
    std::vector<Vertex>   verts;
    std::vector<uint32_t> tris;
};

struct ProceduralMeshSystem::Impl {
    std::unordered_map<uint32_t,MeshData> meshes;
    MeshData* Find(uint32_t id){ auto it=meshes.find(id); return it!=meshes.end()?&it->second:nullptr; }
};

ProceduralMeshSystem::ProceduralMeshSystem(): m_impl(new Impl){}
ProceduralMeshSystem::~ProceduralMeshSystem(){ Shutdown(); delete m_impl; }
void ProceduralMeshSystem::Init(){}
void ProceduralMeshSystem::Shutdown(){ m_impl->meshes.clear(); }
void ProceduralMeshSystem::Reset(){ m_impl->meshes.clear(); }

void ProceduralMeshSystem::CreateMesh (uint32_t id){ m_impl->meshes[id]=MeshData{}; }
void ProceduralMeshSystem::DestroyMesh(uint32_t id){ m_impl->meshes.erase(id); }
void ProceduralMeshSystem::Clear      (uint32_t id){ auto* m=m_impl->Find(id); if(m){m->verts.clear();m->tris.clear();} }
void ProceduralMeshSystem::MergeMesh  (uint32_t dstId, uint32_t srcId){
    auto* dst=m_impl->Find(dstId); auto* src=m_impl->Find(srcId); if(!dst||!src) return;
    uint32_t base=(uint32_t)dst->verts.size();
    dst->verts.insert(dst->verts.end(),src->verts.begin(),src->verts.end());
    for(auto t:src->tris) dst->tris.push_back(t+base);
}

uint32_t ProceduralMeshSystem::AddVertex(uint32_t id,float x,float y,float z,
                                          float nx,float ny,float nz,float u,float v){
    auto* m=m_impl->Find(id); if(!m) return 0;
    Vertex vt{x,y,z,nx,ny,nz,u,v}; m->verts.push_back(vt);
    return (uint32_t)m->verts.size()-1;
}
void ProceduralMeshSystem::AddTriangle(uint32_t id, uint32_t a, uint32_t b, uint32_t c){
    auto* m=m_impl->Find(id); if(!m) return;
    m->tris.push_back(a); m->tris.push_back(b); m->tris.push_back(c);
}

void ProceduralMeshSystem::BuildBox(uint32_t id, float w, float h, float d){
    Clear(id);
    float hw=w*.5f,hh=h*.5f,hd=d*.5f;
    // 6 faces, 4 verts each
    struct Face { float nx,ny,nz; float pts[4][3]; };
    const Face faces[6]={
        { 0,1,0,{{-hw,hh,-hd},{hw,hh,-hd},{hw,hh,hd},{-hw,hh,hd}}},
        { 0,-1,0,{{-hw,-hh,hd},{hw,-hh,hd},{hw,-hh,-hd},{-hw,-hh,-hd}}},
        { 0,0,1,{{-hw,-hh,hd},{hw,-hh,hd},{hw,hh,hd},{-hw,hh,hd}}},
        { 0,0,-1,{{hw,-hh,-hd},{-hw,-hh,-hd},{-hw,hh,-hd},{hw,hh,-hd}}},
        { 1,0,0,{{hw,-hh,hd},{hw,-hh,-hd},{hw,hh,-hd},{hw,hh,hd}}},
        {-1,0,0,{{-hw,-hh,-hd},{-hw,-hh,hd},{-hw,hh,hd},{-hw,hh,-hd}}}
    };
    float uvs[4][2]={{0,0},{1,0},{1,1},{0,1}};
    for(auto& f:faces){
        uint32_t base=(uint32_t)m_impl->Find(id)->verts.size();
        for(int i=0;i<4;i++) AddVertex(id,f.pts[i][0],f.pts[i][1],f.pts[i][2],f.nx,f.ny,f.nz,uvs[i][0],uvs[i][1]);
        AddTriangle(id,base,base+1,base+2); AddTriangle(id,base,base+2,base+3);
    }
}

void ProceduralMeshSystem::BuildSphere(uint32_t id, float r, uint32_t rings, uint32_t slices){
    Clear(id);
    for(uint32_t ri=0;ri<=rings;ri++){
        float phi=kPI*(float)ri/rings;
        for(uint32_t si=0;si<=slices;si++){
            float theta=2*kPI*(float)si/slices;
            float x=std::sin(phi)*std::cos(theta);
            float y=std::cos(phi);
            float z=std::sin(phi)*std::sin(theta);
            AddVertex(id,x*r,y*r,z*r,x,y,z,(float)si/slices,(float)ri/rings);
        }
    }
    for(uint32_t ri=0;ri<rings;ri++) for(uint32_t si=0;si<slices;si++){
        uint32_t a=ri*(slices+1)+si, b=a+1, c=a+(slices+1), e=c+1;
        AddTriangle(id,a,b,e); AddTriangle(id,a,e,c);
    }
}

void ProceduralMeshSystem::BuildCylinder(uint32_t id, float r, float h, uint32_t sl){
    Clear(id);
    float hh=h*.5f;
    for(uint32_t i=0;i<=sl;i++){
        float t=2*kPI*(float)i/sl;
        float cx=std::cos(t), cz=std::sin(t);
        AddVertex(id,cx*r,-hh,cz*r,cx,0,cz,(float)i/sl,0);
        AddVertex(id,cx*r, hh,cz*r,cx,0,cz,(float)i/sl,1);
    }
    for(uint32_t i=0;i<sl;i++){
        uint32_t a=i*2,b=a+1,c=(i+1)*2,e=c+1;
        AddTriangle(id,a,b,e); AddTriangle(id,a,e,c);
    }
}

void ProceduralMeshSystem::BuildPlane(uint32_t id, float w, float h,
                                       uint32_t sx, uint32_t sy){
    Clear(id);
    float hw=w*.5f, hh=h*.5f;
    for(uint32_t y2=0;y2<=sy;y2++) for(uint32_t x2=0;x2<=sx;x2++){
        float u=(float)x2/sx, v=(float)y2/sy;
        AddVertex(id,u*w-hw,0,v*h-hh,0,1,0,u,v);
    }
    for(uint32_t y2=0;y2<sy;y2++) for(uint32_t x2=0;x2<sx;x2++){
        uint32_t a=y2*(sx+1)+x2, b=a+1, c=a+(sx+1), e=c+1;
        AddTriangle(id,a,b,e); AddTriangle(id,a,e,c);
    }
}

void ProceduralMeshSystem::BuildTorus(uint32_t id, float majR, float minR,
                                       uint32_t ms, uint32_t ns){
    Clear(id);
    for(uint32_t i=0;i<=ms;i++) for(uint32_t j=0;j<=ns;j++){
        float a=2*kPI*(float)i/ms, b=2*kPI*(float)j/ns;
        float cx=(majR+minR*std::cos(b))*std::cos(a);
        float cy=minR*std::sin(b);
        float cz=(majR+minR*std::cos(b))*std::sin(a);
        float nx=std::cos(b)*std::cos(a), ny=std::sin(b), nz=std::cos(b)*std::sin(a);
        AddVertex(id,cx,cy,cz,nx,ny,nz,(float)i/ms,(float)j/ns);
    }
    for(uint32_t i=0;i<ms;i++) for(uint32_t j=0;j<ns;j++){
        uint32_t a=i*(ns+1)+j, bv=a+1, c=(i+1)*(ns+1)+j, e=c+1;
        AddTriangle(id,a,bv,e); AddTriangle(id,a,e,c);
    }
}

void ProceduralMeshSystem::Extrude(uint32_t id, const std::vector<float>& px,
                                    const std::vector<float>& py, float depth){
    if(px.size()!=py.size()||px.empty()) return;
    uint32_t n=(uint32_t)px.size();
    float dh=depth*.5f;
    for(uint32_t i=0;i<n;i++){
        AddVertex(id,px[i],py[i],-dh,0,0,-1,(float)i/n,0);
        AddVertex(id,px[i],py[i], dh,0,0, 1,(float)i/n,1);
    }
    for(uint32_t i=0;i<n;i++){
        uint32_t a=i*2,b=a+1,c=((i+1)%n)*2,e=c+1;
        AddTriangle(id,a,b,e); AddTriangle(id,a,e,c);
    }
}

void ProceduralMeshSystem::RecalcNormals(uint32_t id, bool /*smooth*/){
    auto* m=m_impl->Find(id); if(!m) return;
    for(auto& v:m->verts){v.nx=0;v.ny=0;v.nz=0;}
    for(uint32_t i=0;i+2<m->tris.size();i+=3){
        auto& va=m->verts[m->tris[i]];
        auto& vb=m->verts[m->tris[i+1]];
        auto& vc=m->verts[m->tris[i+2]];
        float ax=vb.x-va.x,ay=vb.y-va.y,az=vb.z-va.z;
        float bx=vc.x-va.x,by=vc.y-va.y,bz=vc.z-va.z;
        float nx=ay*bz-az*by,ny=az*bx-ax*bz,nz=ax*by-ay*bx;
        for(Vertex* v:{&va,&vb,&vc}){v->nx+=nx;v->ny+=ny;v->nz+=nz;}
    }
    for(auto& v:m->verts){
        float l=std::sqrt(v.nx*v.nx+v.ny*v.ny+v.nz*v.nz);
        if(l>0){v.nx/=l;v.ny/=l;v.nz/=l;}
    }
}

uint32_t ProceduralMeshSystem::GetVertexCount  (uint32_t id) const { auto* m=m_impl->Find(id);return m?(uint32_t)m->verts.size():0; }
uint32_t ProceduralMeshSystem::GetTriangleCount(uint32_t id) const { auto* m=m_impl->Find(id);return m?(uint32_t)m->tris.size()/3:0; }
void ProceduralMeshSystem::GetVertexPos(uint32_t id, uint32_t idx, float& x, float& y, float& z) const {
    auto* m=m_impl->Find(id); if(!m||idx>=m->verts.size()){x=y=z=0;return;}
    x=m->verts[idx].x; y=m->verts[idx].y; z=m->verts[idx].z;
}
void ProceduralMeshSystem::GetVertexUV(uint32_t id, uint32_t idx, float& u, float& v) const {
    auto* m=m_impl->Find(id); if(!m||idx>=m->verts.size()){u=v=0;return;}
    u=m->verts[idx].u; v=m->verts[idx].v;
}

} // namespace Engine
