#include "Engine/AI/NavMesh/NavMeshBuilder/NavMeshBuilder.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <vector>

namespace Engine {

static float Dist3(NMBVec3 a, NMBVec3 b){
    float dx=a.x-b.x,dy=a.y-b.y,dz=a.z-b.z;
    return std::sqrt(dx*dx+dy*dy+dz*dz);
}

struct Polygon {
    std::vector<NMBVec3> verts;
    std::vector<uint32_t> neighbours;
};

struct NavMeshBuilder::Impl {
    NMBVec3 bmin{}, bmax{};
    float   cellSize{0.3f}, cellHeight{0.2f};
    float   agentRadius{0.4f}, agentHeight{2.f};
    float   maxClimb{0.9f}, maxSlopeDeg{45.f};
    std::vector<Polygon> polys;
    // Raw geometry
    struct Soup { std::vector<float> verts; std::vector<uint32_t> tris; uint8_t area; };
    std::vector<Soup> soups;
    std::function<void(bool)> onBuild;
};

NavMeshBuilder::NavMeshBuilder(): m_impl(new Impl){}
NavMeshBuilder::~NavMeshBuilder(){ Shutdown(); delete m_impl; }
void NavMeshBuilder::Init(){}
void NavMeshBuilder::Shutdown(){ Clear(); }
void NavMeshBuilder::Clear(){ m_impl->polys.clear(); m_impl->soups.clear(); }

void NavMeshBuilder::SetBounds   (NMBVec3 mn, NMBVec3 mx){ m_impl->bmin=mn; m_impl->bmax=mx; }
void NavMeshBuilder::SetCellSize (float cs){ m_impl->cellSize=cs; }
void NavMeshBuilder::SetCellHeight(float ch){ m_impl->cellHeight=ch; }
void NavMeshBuilder::SetAgentParams(float r, float h, float mc, float ms){
    m_impl->agentRadius=r; m_impl->agentHeight=h;
    m_impl->maxClimb=mc;   m_impl->maxSlopeDeg=ms;
}
void NavMeshBuilder::AddTriangleSoup(const float* verts, uint32_t vertCount,
                                      const uint32_t* tris, uint32_t triCount,
                                      uint8_t areaId){
    Impl::Soup s;
    s.verts.assign(verts, verts+vertCount*3);
    s.tris .assign(tris,  tris+triCount*3);
    s.area=areaId;
    m_impl->soups.push_back(std::move(s));
}

bool NavMeshBuilder::Build(){
    m_impl->polys.clear();
    // Stub: create one polygon per triangle from the first soup
    for(auto& soup:m_impl->soups){
        uint32_t triCnt=(uint32_t)soup.tris.size()/3;
        for(uint32_t i=0;i<triCnt;i++){
            uint32_t i0=soup.tris[i*3+0]*3;
            uint32_t i1=soup.tris[i*3+1]*3;
            uint32_t i2=soup.tris[i*3+2]*3;
            if(i0+2>=soup.verts.size()||i1+2>=soup.verts.size()||i2+2>=soup.verts.size()) continue;
            Polygon p;
            p.verts.push_back({soup.verts[i0],soup.verts[i0+1],soup.verts[i0+2]});
            p.verts.push_back({soup.verts[i1],soup.verts[i1+1],soup.verts[i1+2]});
            p.verts.push_back({soup.verts[i2],soup.verts[i2+1],soup.verts[i2+2]});
            m_impl->polys.push_back(std::move(p));
        }
    }
    // Build adjacency: share an edge ↔ neighbour
    for(uint32_t i=0;i<m_impl->polys.size();i++){
        for(uint32_t j=i+1;j<m_impl->polys.size();j++){
            // Count shared vertices
            int shared=0;
            for(auto& va:m_impl->polys[i].verts)
                for(auto& vb:m_impl->polys[j].verts)
                    if(Dist3(va,vb)<0.001f) shared++;
            if(shared>=2){
                m_impl->polys[i].neighbours.push_back(j);
                m_impl->polys[j].neighbours.push_back(i);
            }
        }
    }
    bool ok=!m_impl->polys.empty();
    if(m_impl->onBuild) m_impl->onBuild(ok);
    return ok;
}
void NavMeshBuilder::SetOnBuildComplete(std::function<void(bool)> cb){ m_impl->onBuild=cb; }

uint32_t NavMeshBuilder::GetPolyCount() const { return (uint32_t)m_impl->polys.size(); }
uint32_t NavMeshBuilder::GetPolyVerts(uint32_t idx, NMBVec3* out, uint32_t maxV) const {
    if(idx>=m_impl->polys.size()) return 0;
    auto& p=m_impl->polys[idx];
    uint32_t n=std::min((uint32_t)p.verts.size(),maxV);
    for(uint32_t i=0;i<n;i++) out[i]=p.verts[i];
    return n;
}
uint32_t NavMeshBuilder::GetPolyNeighbours(uint32_t idx, uint32_t* out) const {
    if(idx>=m_impl->polys.size()) return 0;
    auto& nb=m_impl->polys[idx].neighbours;
    for(uint32_t i=0;i<nb.size();i++) out[i]=nb[i];
    return (uint32_t)nb.size();
}
NMBVec3 NavMeshBuilder::GetPolyCenter(uint32_t idx) const {
    if(idx>=m_impl->polys.size()) return {};
    auto& p=m_impl->polys[idx];
    NMBVec3 c{}; for(auto& v:p.verts){c.x+=v.x;c.y+=v.y;c.z+=v.z;}
    float n=(float)p.verts.size(); return {c.x/n,c.y/n,c.z/n};
}
bool NavMeshBuilder::IsWalkable(NMBVec3 pos) const {
    for(auto& p:m_impl->polys){
        NMBVec3 c=GetPolyCenter((uint32_t)(&p-m_impl->polys.data()));
        if(Dist3(pos,c)<m_impl->cellSize*2) return true;
    }
    return false;
}
uint32_t NavMeshBuilder::FindPath(NMBVec3 start, NMBVec3 end,
                                   NMBVec3* outPath, uint32_t maxPts) const {
    if(m_impl->polys.empty()||maxPts==0) return 0;
    // Find nearest poly to start/end
    auto nearest=[&](NMBVec3 p)->uint32_t{
        uint32_t best=0; float bestD=1e30f;
        for(uint32_t i=0;i<m_impl->polys.size();i++){
            float d=Dist3(GetPolyCenter(i),p);
            if(d<bestD){bestD=d;best=i;}
        }
        return best;
    };
    uint32_t startPoly=nearest(start), endPoly=nearest(end);
    // BFS
    std::queue<uint32_t> q; q.push(startPoly);
    std::unordered_map<uint32_t,uint32_t> prev; prev[startPoly]=startPoly;
    while(!q.empty()){
        uint32_t cur=q.front(); q.pop();
        if(cur==endPoly) break;
        for(auto nb:m_impl->polys[cur].neighbours){
            if(!prev.count(nb)){ prev[nb]=cur; q.push(nb); }
        }
    }
    // Reconstruct
    std::vector<NMBVec3> path;
    uint32_t cur=endPoly;
    while(cur!=startPoly){
        path.push_back(GetPolyCenter(cur));
        auto it=prev.find(cur); if(it==prev.end()||it->second==cur) break;
        cur=it->second;
    }
    path.push_back(GetPolyCenter(startPoly));
    std::reverse(path.begin(),path.end());
    uint32_t n=std::min((uint32_t)path.size(),maxPts);
    for(uint32_t i=0;i<n;i++) outPath[i]=path[i];
    return n;
}

} // namespace Engine
