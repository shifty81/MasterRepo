#include "Engine/Render/Trail/TrailRenderer/TrailRenderer.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

struct TrailSeg { float pos[3]{}; float time{0.f}; float age{0.f}; };

struct TrailData {
    uint32_t id;
    TrailDesc desc;
    std::vector<TrailSeg> segs;
    std::vector<TrailVertex> verts;
    float uvOffset{0.f};
    float lastAddTime{-1e9f};
    float lastPos[3]{};

    void RebuildVerts(float /*now*/){
        verts.clear();
        if(segs.size()<2) return;
        uint32_t n=(uint32_t)segs.size();
        for(uint32_t i=0;i<n;i++){
            float t=(float)i/(n-1);
            float alpha=desc.fadeOverLifetime?(1.f-segs[i].age/desc.segmentLifetime):1.f;
            float w=desc.width*(desc.taperWidth?(1.f-t):1.f);
            // Compute perpendicular (simplified: use X axis)
            float perp[3]={w*0.5f,0,0};
            uint32_t ni=std::min(i+1,n-1);
            float dx=segs[ni].pos[0]-segs[i].pos[0];
            float dz=segs[ni].pos[2]-segs[i].pos[2];
            float len=std::sqrt(dx*dx+dz*dz)+1e-9f;
            perp[0]=-dz/len*w*0.5f; perp[2]=dx/len*w*0.5f;
            // Lerp colour
            float* hc=const_cast<float*>(desc.headColour);
            float* tc=const_cast<float*>(desc.tailColour);
            float col[4]; for(int k=0;k<4;k++) col[k]=hc[k]+(tc[k]-hc[k])*t;
            float u=t+uvOffset;
            // Two verts (ribbon side)
            TrailVertex va,vb;
            for(int k=0;k<3;k++){va.pos[k]=segs[i].pos[k]+perp[k]; vb.pos[k]=segs[i].pos[k]-perp[k];}
            va.uv[0]=u; va.uv[1]=0; vb.uv[0]=u; vb.uv[1]=1;
            for(int k=0;k<4;k++){va.colour[k]=col[k]*alpha; vb.colour[k]=col[k]*alpha;}
            verts.push_back(va); verts.push_back(vb);
        }
    }
};

struct TrailRenderer::Impl {
    std::vector<TrailData*> trails;
    uint32_t nextId{1};
    static const std::vector<TrailVertex> empty;

    TrailData* Find(uint32_t id){ for(auto* t:trails) if(t->id==id) return t; return nullptr; }
    const TrailData* Find(uint32_t id) const { for(auto* t:trails) if(t->id==id) return t; return nullptr; }
};
const std::vector<TrailVertex> TrailRenderer::Impl::empty;

TrailRenderer::TrailRenderer()  : m_impl(new Impl){}
TrailRenderer::~TrailRenderer() { Shutdown(); delete m_impl; }
void TrailRenderer::Init()     {}
void TrailRenderer::Shutdown() { for(auto* t:m_impl->trails) delete t; m_impl->trails.clear(); }

uint32_t TrailRenderer::Create(const TrailDesc& d){
    TrailData* td=new TrailData; td->id=m_impl->nextId++; td->desc=d;
    m_impl->trails.push_back(td); return td->id;
}
void TrailRenderer::Destroy(uint32_t id){
    auto& v=m_impl->trails;
    for(auto it=v.begin();it!=v.end();++it) if((*it)->id==id){delete *it;v.erase(it);return;}
}
bool TrailRenderer::Has(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void TrailRenderer::AddPoint(uint32_t id, const float pos[3], float time){
    auto* td=m_impl->Find(id); if(!td) return;
    // Distance gate
    if(td->segs.size()>0){
        float dx=pos[0]-td->lastPos[0], dy=pos[1]-td->lastPos[1], dz=pos[2]-td->lastPos[2];
        if(std::sqrt(dx*dx+dy*dy+dz*dz)<td->desc.minDistance) return;
    }
    // Remove if at max
    while((uint32_t)td->segs.size()>=td->desc.maxSegments) td->segs.erase(td->segs.begin());
    TrailSeg s; for(int i=0;i<3;i++) s.pos[i]=pos[i]; s.time=time; s.age=0.f;
    td->segs.push_back(s);
    for(int i=0;i<3;i++) td->lastPos[i]=pos[i];
}
void TrailRenderer::Clear(uint32_t id){ auto* td=m_impl->Find(id); if(td) td->segs.clear(); }

const std::vector<TrailVertex>& TrailRenderer::GetVertices(uint32_t id) const {
    const auto* td=m_impl->Find(id); return td?td->verts:Impl::empty;
}
uint32_t TrailRenderer::VertexCount (uint32_t id) const { auto* td=m_impl->Find(id); return td?(uint32_t)td->verts.size():0; }
uint32_t TrailRenderer::SegmentCount(uint32_t id) const { auto* td=m_impl->Find(id); return td?(uint32_t)td->segs.size():0; }

void TrailRenderer::SetWidth        (uint32_t id, float w){ auto* t=m_impl->Find(id); if(t) t->desc.width=w; }
void TrailRenderer::SetLifetime     (uint32_t id, float s){ auto* t=m_impl->Find(id); if(t) t->desc.segmentLifetime=s; }
void TrailRenderer::SetHeadColour   (uint32_t id, const float rgba[4]){ auto* t=m_impl->Find(id); if(t) for(int i=0;i<4;i++) t->desc.headColour[i]=rgba[i]; }
void TrailRenderer::SetTailColour   (uint32_t id, const float rgba[4]){ auto* t=m_impl->Find(id); if(t) for(int i=0;i<4;i++) t->desc.tailColour[i]=rgba[i]; }
void TrailRenderer::SetUVScrollSpeed(uint32_t id, float s){ auto* t=m_impl->Find(id); if(t) t->desc.uvScrollSpeed=s; }

std::vector<uint32_t> TrailRenderer::GetAll() const {
    std::vector<uint32_t> out; for(auto* t:m_impl->trails) out.push_back(t->id); return out;
}

void TrailRenderer::Tick(float dt){
    for(auto* td:m_impl->trails){
        td->uvOffset+=td->desc.uvScrollSpeed*dt;
        // Age segments
        for(auto& s:td->segs) s.age+=dt;
        // Remove expired
        td->segs.erase(std::remove_if(td->segs.begin(),td->segs.end(),
            [&](const TrailSeg& s){ return s.age>=td->desc.segmentLifetime; }),td->segs.end());
        td->RebuildVerts(0.f);
    }
}

} // namespace Engine
