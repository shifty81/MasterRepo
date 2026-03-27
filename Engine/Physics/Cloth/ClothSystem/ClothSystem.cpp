#include "Engine/Physics/Cloth/ClothSystem/ClothSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct Vertex {
    float pos[3]{}, prevPos[3]{}, vel[3]{};
    float invMass{1.f};
    bool  pinned{false};
    float pinnedPos[3]{};
};

struct Edge {
    uint32_t a, b;
    float    restLen;
    float    stiffness;
    bool     torn{false};
};

struct ClothData {
    uint32_t id{0};
    ClothDesc desc;
    std::vector<Vertex> verts;
    std::vector<Edge>   stretch, shear, bend;
    float gravity[3]{0,-9.81f,0};
    float wind[3]{};
    std::vector<ClothSphereProxy> spheres;
    std::vector<ClothPlaneProxy>  planes;
    uint32_t rows{0}, cols{0};
};

static float Dot3(const float a[3], const float b[3]){
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
static float Len3(const float v[3]){
    return std::sqrt(Dot3(v,v));
}

struct ClothSystem::Impl {
    std::vector<ClothData> cloths;
    uint32_t nextId{1};
    std::function<void(uint32_t,uint32_t)> onTear;

    ClothData* Find(uint32_t id){
        for(auto& c:cloths) if(c.id==id) return &c; return nullptr;
    }
    const ClothData* Find(uint32_t id) const {
        for(auto& c:cloths) if(c.id==id) return &c; return nullptr;
    }

    void AddEdge(ClothData& c, std::vector<Edge>& list, uint32_t a, uint32_t b, float stiff) {
        float dx=c.verts[a].pos[0]-c.verts[b].pos[0];
        float dy=c.verts[a].pos[1]-c.verts[b].pos[1];
        float dz=c.verts[a].pos[2]-c.verts[b].pos[2];
        float len=std::sqrt(dx*dx+dy*dy+dz*dz);
        list.push_back({a,b,len,stiff});
    }

    void SolveEdge(ClothData& c, Edge& e) {
        if(e.torn) return;
        Vertex& va=c.verts[e.a]; Vertex& vb=c.verts[e.b];
        float d[3]={vb.pos[0]-va.pos[0], vb.pos[1]-va.pos[1], vb.pos[2]-va.pos[2]};
        float len=Len3(d)+1e-9f;
        float ratio=len/e.restLen;
        if(ratio>c.desc.tearThreshold) {
            e.torn=true;
            if(onTear) onTear(c.id, (uint32_t)(&e-c.stretch.data()));
            return;
        }
        float diff=(len-e.restLen)/len * e.stiffness;
        float wSum=va.invMass+vb.invMass+1e-9f;
        for(int i=0;i<3;i++){
            if(!va.pinned) va.pos[i]+=va.invMass/wSum*diff*d[i];
            if(!vb.pinned) vb.pos[i]-=vb.invMass/wSum*diff*d[i];
        }
    }
};

ClothSystem::ClothSystem()  : m_impl(new Impl){}
ClothSystem::~ClothSystem() { Shutdown(); delete m_impl; }

void ClothSystem::Init()     {}
void ClothSystem::Shutdown() { m_impl->cloths.clear(); }

uint32_t ClothSystem::CreateCloth(const ClothDesc& desc, const float origin[3])
{
    ClothData c; c.id=m_impl->nextId++; c.desc=desc;
    c.rows=desc.rows; c.cols=desc.cols;
    float ox=origin?origin[0]:0.f, oy=origin?origin[1]:0.f, oz=origin?origin[2]:0.f;
    // Create grid
    for(uint32_t r=0;r<desc.rows;r++) for(uint32_t col=0;col<desc.cols;col++){
        Vertex v;
        v.pos[0]=ox+col*desc.spacing; v.pos[1]=oy; v.pos[2]=oz+r*desc.spacing;
        for(int i=0;i<3;i++) v.prevPos[i]=v.pos[i];
        v.invMass=1.f/desc.mass;
        c.verts.push_back(v);
    }
    // Stretch edges (horizontal + vertical)
    auto idx=[&](uint32_t r,uint32_t col){return r*desc.cols+col;};
    for(uint32_t r=0;r<desc.rows;r++) for(uint32_t col=0;col<desc.cols;col++){
        if(col+1<desc.cols) m_impl->AddEdge(c,c.stretch,idx(r,col),idx(r,col+1),desc.stretchStiff);
        if(r+1<desc.rows)   m_impl->AddEdge(c,c.stretch,idx(r,col),idx(r+1,col),desc.stretchStiff);
    }
    // Shear
    for(uint32_t r=0;r+1<desc.rows;r++) for(uint32_t col=0;col+1<desc.cols;col++){
        m_impl->AddEdge(c,c.shear,idx(r,col),idx(r+1,col+1),desc.shearStiff);
        m_impl->AddEdge(c,c.shear,idx(r,col+1),idx(r+1,col),desc.shearStiff);
    }
    // Bend
    for(uint32_t r=0;r<desc.rows;r++) for(uint32_t col=0;col<desc.cols;col++){
        if(col+2<desc.cols) m_impl->AddEdge(c,c.bend,idx(r,col),idx(r,col+2),desc.bendStiff);
        if(r+2<desc.rows)   m_impl->AddEdge(c,c.bend,idx(r,col),idx(r+2,col),desc.bendStiff);
    }
    m_impl->cloths.push_back(std::move(c));
    return m_impl->cloths.back().id;
}

void ClothSystem::DestroyCloth(uint32_t id){
    auto& v=m_impl->cloths;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& c){return c.id==id;}),v.end());
}
bool ClothSystem::HasCloth(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void ClothSystem::SetGravity(uint32_t id, const float g[3]){
    auto* c=m_impl->Find(id); if(c) for(int i=0;i<3;i++) c->gravity[i]=g[i];
}
void ClothSystem::SetWind(uint32_t id, const float w[3]){
    auto* c=m_impl->Find(id); if(c) for(int i=0;i<3;i++) c->wind[i]=w[i];
}

void ClothSystem::Pin(uint32_t id, uint32_t vi, const float wp[3]){
    auto* c=m_impl->Find(id); if(c&&vi<c->verts.size()){
        auto& v=c->verts[vi]; v.pinned=true;
        for(int i=0;i<3;i++){v.pinnedPos[i]=wp[i]; v.pos[i]=wp[i]; v.prevPos[i]=wp[i];}
    }
}
void ClothSystem::Unpin(uint32_t id, uint32_t vi){
    auto* c=m_impl->Find(id); if(c&&vi<c->verts.size()) c->verts[vi].pinned=false;
}

void ClothSystem::AddImpulse(uint32_t id, uint32_t vi, const float imp[3]){
    auto* c=m_impl->Find(id);
    if(c&&vi<c->verts.size()&&!c->verts[vi].pinned)
        for(int i=0;i<3;i++) c->verts[vi].vel[i]+=imp[i]*c->verts[vi].invMass;
}

void ClothSystem::AddSphere(uint32_t id, const ClothSphereProxy& s){
    auto* c=m_impl->Find(id); if(c) c->spheres.push_back(s);
}
void ClothSystem::AddPlane(uint32_t id, const ClothPlaneProxy& p){
    auto* c=m_impl->Find(id); if(c) c->planes.push_back(p);
}
void ClothSystem::ClearColliders(uint32_t id){
    auto* c=m_impl->Find(id); if(c){c->spheres.clear();c->planes.clear();}
}

const float* ClothSystem::GetVertexPos(uint32_t id, uint32_t vi) const {
    const auto* c=m_impl->Find(id);
    return (c&&vi<c->verts.size())?c->verts[vi].pos:nullptr;
}
uint32_t ClothSystem::VertexCount(uint32_t id) const {
    const auto* c=m_impl->Find(id); return c?(uint32_t)c->verts.size():0;
}

void ClothSystem::SetOnTear(std::function<void(uint32_t,uint32_t)> cb){ m_impl->onTear=cb; }
std::vector<uint32_t> ClothSystem::GetAll() const {
    std::vector<uint32_t> out; for(auto& c:m_impl->cloths) out.push_back(c.id); return out;
}

void ClothSystem::Tick(float dt)
{
    for(auto& c : m_impl->cloths) {
        // Verlet integration
        for(auto& v : c.verts) {
            if(v.pinned){for(int i=0;i<3;i++) v.pos[i]=v.pinnedPos[i]; continue;}
            float acc[3];
            for(int i=0;i<3;i++) acc[i]=c.gravity[i]+c.wind[i];
            float newPos[3];
            for(int i=0;i<3;i++)
                newPos[i]=v.pos[i]+(1.f-c.desc.damping)*(v.pos[i]-v.prevPos[i])+acc[i]*dt*dt;
            for(int i=0;i<3;i++){v.prevPos[i]=v.pos[i]; v.pos[i]=newPos[i];}
        }
        // Constraint solving (5 iterations)
        for(int iter=0;iter<5;iter++){
            for(auto& e:c.stretch) m_impl->SolveEdge(c,e);
            for(auto& e:c.shear)   m_impl->SolveEdge(c,e);
            for(auto& e:c.bend)    m_impl->SolveEdge(c,e);
        }
        // Sphere collisions
        for(auto& v:c.verts){if(v.pinned) continue;
            for(auto& s:c.spheres){
                float d[3]={v.pos[0]-s.centre[0],v.pos[1]-s.centre[1],v.pos[2]-s.centre[2]};
                float l=Len3(d)+1e-9f;
                if(l<s.radius){float f=(s.radius-l)/l;for(int i=0;i<3;i++) v.pos[i]+=d[i]*f;}
            }
            for(auto& p:c.planes){
                float dist=Dot3(v.pos,p.normal)-p.d;
                if(dist<0.f) for(int i=0;i<3;i++) v.pos[i]-=p.normal[i]*dist;
            }
        }
    }
}

} // namespace Engine
