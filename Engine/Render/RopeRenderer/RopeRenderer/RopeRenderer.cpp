#include "Engine/Render/RopeRenderer/RopeRenderer/RopeRenderer.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct RopeNode {
    RRVec3 pos{};
    RRVec3 prevPos{};
    bool   fixed{false};
};

struct RopeData {
    uint32_t       id;
    uint32_t       segCount{16};
    float          thickness{0.05f};
    float          sagFactor{0.3f};
    float          twistAngle{0};
    float          uvTile{1.f};
    bool           caps{false};
    RRVec3         start{0,0,0}, end{1,0,0};
    bool           startFixed{true}, endFixed{true};
    std::vector<RopeNode> nodes;
    bool           dirty{true};
};

struct RopeRenderer::Impl {
    std::unordered_map<uint32_t,RopeData> ropes;

    RopeData* Find(uint32_t id){ auto it=ropes.find(id); return it!=ropes.end()?&it->second:nullptr; }
    const RopeData* Find(uint32_t id) const { auto it=ropes.find(id); return it!=ropes.end()?&it->second:nullptr; }

    static void InitNodes(RopeData& r){
        uint32_t n=r.segCount+1;
        r.nodes.resize(n);
        for(uint32_t i=0;i<n;i++){
            float t=(float)i/r.segCount;
            r.nodes[i].pos={r.start.x+(r.end.x-r.start.x)*t,
                             r.start.y+(r.end.y-r.start.y)*t-r.sagFactor*(1-4*(t-0.5f)*(t-0.5f)),
                             r.start.z+(r.end.z-r.start.z)*t};
            r.nodes[i].prevPos=r.nodes[i].pos;
        }
        r.nodes.front().fixed=r.startFixed;
        r.nodes.back().fixed=r.endFixed;
        r.dirty=false;
    }
};

RopeRenderer::RopeRenderer(): m_impl(new Impl){}
RopeRenderer::~RopeRenderer(){ Shutdown(); delete m_impl; }
void RopeRenderer::Init(){}
void RopeRenderer::Shutdown(){ m_impl->ropes.clear(); }
void RopeRenderer::Reset(){ m_impl->ropes.clear(); }

void RopeRenderer::CreateRope(uint32_t id, uint32_t segs, float thick){
    RopeData r; r.id=id; r.segCount=segs; r.thickness=thick;
    m_impl->ropes[id]=r;
}
void RopeRenderer::DestroyRope(uint32_t id){ m_impl->ropes.erase(id); }

void RopeRenderer::SetEndpoints(uint32_t id, RRVec3 s, RRVec3 e){
    auto* r=m_impl->Find(id); if(!r) return;
    r->start=s; r->end=e; r->dirty=true;
}
void RopeRenderer::SetSag(uint32_t id, float f){ auto* r=m_impl->Find(id); if(r){ r->sagFactor=f; r->dirty=true; } }
void RopeRenderer::SetTwist(uint32_t id, float a){ auto* r=m_impl->Find(id); if(r) r->twistAngle=a; }
void RopeRenderer::SetSegmentCount(uint32_t id, uint32_t n){
    auto* r=m_impl->Find(id); if(r){ r->segCount=n; r->dirty=true; }
}
void RopeRenderer::SetThickness(uint32_t id, float t){ auto* r=m_impl->Find(id); if(r) r->thickness=t; }
void RopeRenderer::SetUVTile   (uint32_t id, float u){ auto* r=m_impl->Find(id); if(r) r->uvTile=u; }
void RopeRenderer::EnableCaps  (uint32_t id, bool on){ auto* r=m_impl->Find(id); if(r) r->caps=on; }

void RopeRenderer::FixEndpoint(uint32_t id, uint32_t end, bool fixed){
    auto* r=m_impl->Find(id); if(!r) return;
    if(end==0) r->startFixed=fixed;
    else        r->endFixed  =fixed;
    if(!r->nodes.empty()){
        r->nodes.front().fixed=r->startFixed;
        r->nodes.back() .fixed=r->endFixed;
    }
}

void RopeRenderer::UpdateSimulation(uint32_t id, float dt, RRVec3 gravity){
    auto* r=m_impl->Find(id); if(!r) return;
    if(r->dirty) Impl::InitNodes(*r);
    uint32_t n=(uint32_t)r->nodes.size();
    uint32_t iters=8;
    // Verlet integration
    for(auto& node:r->nodes){
        if(node.fixed) continue;
        RRVec3 vel={node.pos.x-node.prevPos.x, node.pos.y-node.prevPos.y, node.pos.z-node.prevPos.z};
        node.prevPos=node.pos;
        node.pos.x+=vel.x+gravity.x*dt*dt;
        node.pos.y+=vel.y+gravity.y*dt*dt;
        node.pos.z+=vel.z+gravity.z*dt*dt;
    }
    // Enforce fixed endpoints
    r->nodes.front().pos=r->start;
    r->nodes.back() .pos=r->end;

    // Distance constraints
    float segLen=std::sqrt(
        (r->end.x-r->start.x)*(r->end.x-r->start.x)+
        (r->end.y-r->start.y)*(r->end.y-r->start.y)+
        (r->end.z-r->start.z)*(r->end.z-r->start.z))/r->segCount;

    for(uint32_t iter=0;iter<iters;iter++){
        for(uint32_t i=0;i+1<n;i++){
            auto& a=r->nodes[i]; auto& b=r->nodes[i+1];
            float dx=b.pos.x-a.pos.x, dy=b.pos.y-a.pos.y, dz=b.pos.z-a.pos.z;
            float d=std::sqrt(dx*dx+dy*dy+dz*dz);
            if(d<1e-6f) continue;
            float err=(d-segLen)/d*0.5f;
            if(!a.fixed){ a.pos.x+=dx*err; a.pos.y+=dy*err; a.pos.z+=dz*err; }
            if(!b.fixed){ b.pos.x-=dx*err; b.pos.y-=dy*err; b.pos.z-=dz*err; }
        }
        r->nodes.front().pos=r->start;
        r->nodes.back() .pos=r->end;
    }
}

static inline RRVec3 Cross3(RRVec3 a, RRVec3 b){
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
static inline float Len3r(RRVec3 v){ return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); }
static inline RRVec3 Norm3r(RRVec3 v){ float l=Len3r(v); return l>0?RRVec3{v.x/l,v.y/l,v.z/l}:RRVec3{0,1,0}; }

void RopeRenderer::GetVertices(uint32_t id,
                                std::vector<RRVec3>& outPos,
                                std::vector<RRVec3>& outNorm,
                                std::vector<RRVec2>& outUV) const {
    auto* r=m_impl->Find(id); if(!r||r->nodes.empty()){ return; }
    const uint32_t sides=8;
    uint32_t n=(uint32_t)r->nodes.size();
    outPos.clear(); outNorm.clear(); outUV.clear();
    for(uint32_t i=0;i<n;i++){
        float t=(float)i/(n-1);
        // Compute tangent
        RRVec3 tang;
        if(i+1<n){
            tang={r->nodes[i+1].pos.x-r->nodes[i].pos.x,
                  r->nodes[i+1].pos.y-r->nodes[i].pos.y,
                  r->nodes[i+1].pos.z-r->nodes[i].pos.z};
        } else {
            tang={r->nodes[i].pos.x-r->nodes[i-1].pos.x,
                  r->nodes[i].pos.y-r->nodes[i-1].pos.y,
                  r->nodes[i].pos.z-r->nodes[i-1].pos.z};
        }
        tang=Norm3r(tang);
        // Find perpendicular
        RRVec3 up={0,1,0};
        if(std::abs(tang.y)>0.99f) up={1,0,0};
        RRVec3 right=Norm3r(Cross3(tang,up));
        up=Norm3r(Cross3(right,tang));

        for(uint32_t s=0;s<sides;s++){
            float a=2*3.14159265f*s/sides + r->twistAngle*t;
            float cosA=std::cos(a), sinA=std::sin(a);
            RRVec3 nr={right.x*cosA+up.x*sinA, right.y*cosA+up.y*sinA, right.z*cosA+up.z*sinA};
            RRVec3 pos={r->nodes[i].pos.x+nr.x*r->thickness,
                        r->nodes[i].pos.y+nr.y*r->thickness,
                        r->nodes[i].pos.z+nr.z*r->thickness};
            outPos.push_back(pos);
            outNorm.push_back(nr);
            outUV.push_back({t*r->uvTile, (float)s/sides});
        }
    }
}

void RopeRenderer::GetIndices(uint32_t id, std::vector<uint32_t>& outIdx) const {
    auto* r=m_impl->Find(id); if(!r||r->nodes.empty()) return;
    const uint32_t sides=8;
    uint32_t n=(uint32_t)r->nodes.size();
    outIdx.clear();
    for(uint32_t i=0;i+1<n;i++){
        for(uint32_t s=0;s<sides;s++){
            uint32_t a=i*sides+s, b=i*sides+(s+1)%sides;
            uint32_t c=(i+1)*sides+s, d=(i+1)*sides+(s+1)%sides;
            outIdx.push_back(a); outIdx.push_back(c); outIdx.push_back(b);
            outIdx.push_back(b); outIdx.push_back(c); outIdx.push_back(d);
        }
    }
}

RRVec3 RopeRenderer::GetMidpointPos(uint32_t id) const {
    auto* r=m_impl->Find(id);
    if(!r||r->nodes.empty()) return {0,0,0};
    uint32_t mid=r->nodes.size()/2;
    return r->nodes[mid].pos;
}

} // namespace Engine
