#include "Engine/Spline/SplineSystem.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Engine {

// ── Arc-length table ──────────────────────────────────────────────────────────

struct ArcEntry { float t{0.f}; float length{0.f}; };

struct SplineData {
    SplineInfo              info;
    std::vector<SplinePoint> points;
    std::vector<ArcEntry>   arcTable;
    float                   totalLen{0.f};

    static float Lerp(float a, float b, float t){ return a*(1-t)+b*t; }

    void EvalCatmullRom(float t, float pos[3], float tang[3]) const {
        if(points.size()<2){ for(int i=0;i<3;++i){ pos[i]=0; if(tang) tang[i]=0; } return; }
        uint32_t n=(uint32_t)points.size();
        float ft=t*((float)(info.closed?n:n-1));
        int   seg=(int)ft; float u=ft-seg;
        auto wrap=[&](int i)->int{ return info.closed?((i%n+n)%n):std::max(0,std::min((int)n-1,i)); };
        auto& p0=points[wrap(seg-1)].pos;
        auto& p1=points[wrap(seg  )].pos;
        auto& p2=points[wrap(seg+1)].pos;
        auto& p3=points[wrap(seg+2)].pos;
        float u2=u*u, u3=u2*u;
        for(int i=0;i<3;++i){
            pos[i]=0.5f*((-p0[i]+3*p1[i]-3*p2[i]+p3[i])*u3
                        +(2*p0[i]-5*p1[i]+4*p2[i]-p3[i])*u2
                        +(-p0[i]+p2[i])*u
                        +2*p1[i]);
            if(tang) tang[i]=0.5f*((-p0[i]+3*p1[i]-3*p2[i]+p3[i])*3*u2
                                  +(2*p0[i]-5*p1[i]+4*p2[i]-p3[i])*2*u
                                  +(-p0[i]+p2[i]));
        }
    }

    void BuildArcTable(int steps=200){
        arcTable.clear(); arcTable.push_back({0.f,0.f});
        float prevPos[3]; EvalCatmullRom(0.f,prevPos,nullptr);
        totalLen=0.f;
        for(int i=1;i<=steps;++i){
            float ft=(float)i/(float)steps;
            float pos[3]; EvalCatmullRom(ft,pos,nullptr);
            float dx=pos[0]-prevPos[0],dy=pos[1]-prevPos[1],dz=pos[2]-prevPos[2];
            totalLen+=std::sqrt(dx*dx+dy*dy+dz*dz);
            arcTable.push_back({ft,totalLen});
            for(int j=0;j<3;++j) prevPos[j]=pos[j];
        }
        info.totalLength=totalLen;
    }

    float DistanceToT(float dist) const {
        if(arcTable.empty()) return 0.f;
        dist=std::max(0.f,std::min(dist,totalLen));
        size_t lo=0,hi=arcTable.size()-1;
        while(lo+1<hi){ size_t mid=(lo+hi)/2; if(arcTable[mid].length<=dist) lo=mid; else hi=mid; }
        if(arcTable[hi].length==arcTable[lo].length) return arcTable[lo].t;
        float frac=(dist-arcTable[lo].length)/(arcTable[hi].length-arcTable[lo].length);
        return arcTable[lo].t+frac*(arcTable[hi].t-arcTable[lo].t);
    }
};

struct SplineSystem::Impl {
    std::unordered_map<uint32_t,SplineData> splines;
    uint32_t nextId{1};
};

SplineSystem::SplineSystem() : m_impl(new Impl()) {}
SplineSystem::~SplineSystem() { delete m_impl; }
void SplineSystem::Init()     {}
void SplineSystem::Shutdown() { m_impl->splines.clear(); }

uint32_t SplineSystem::CreateSpline(const std::string& name, SplineType type, bool closed) {
    uint32_t id=m_impl->nextId++;
    SplineData& d=m_impl->splines[id];
    d.info.id=id; d.info.name=name; d.info.type=type; d.info.closed=closed;
    return id;
}
void SplineSystem::DestroySpline(uint32_t id){ m_impl->splines.erase(id); }
bool SplineSystem::HasSpline(uint32_t id) const{ return m_impl->splines.count(id)>0; }
SplineInfo SplineSystem::GetInfo(uint32_t id) const{
    auto it=m_impl->splines.find(id); return it!=m_impl->splines.end()?it->second.info:SplineInfo{};
}
std::vector<SplineInfo> SplineSystem::AllSplines() const{
    std::vector<SplineInfo> out; for(auto&[k,v]:m_impl->splines) out.push_back(v.info); return out;
}

void SplineSystem::AddPoint(uint32_t id, const float pos[3]){
    auto it=m_impl->splines.find(id); if(it==m_impl->splines.end()) return;
    SplinePoint p; for(int i=0;i<3;++i) p.pos[i]=pos[i];
    it->second.points.push_back(p); it->second.info.pointCount++;
    it->second.BuildArcTable();
}
void SplineSystem::InsertPoint(uint32_t id, uint32_t index, const float pos[3]){
    auto it=m_impl->splines.find(id); if(it==m_impl->splines.end()) return;
    SplinePoint p; for(int i=0;i<3;++i) p.pos[i]=pos[i];
    auto& pts=it->second.points;
    if(index>=(uint32_t)pts.size()) pts.push_back(p); else pts.insert(pts.begin()+index,p);
    it->second.info.pointCount=(uint32_t)pts.size();
    it->second.BuildArcTable();
}
void SplineSystem::SetPoint(uint32_t id, uint32_t index, const SplinePoint& pt){
    auto it=m_impl->splines.find(id); if(it==m_impl->splines.end()) return;
    if(index<(uint32_t)it->second.points.size()) it->second.points[index]=pt;
    it->second.BuildArcTable();
}
void SplineSystem::RemovePoint(uint32_t id, uint32_t index){
    auto it=m_impl->splines.find(id); if(it==m_impl->splines.end()) return;
    auto& pts=it->second.points;
    if(index<(uint32_t)pts.size()){ pts.erase(pts.begin()+index); it->second.info.pointCount--; }
    it->second.BuildArcTable();
}
SplinePoint SplineSystem::GetPoint(uint32_t id, uint32_t index) const{
    auto it=m_impl->splines.find(id); if(it==m_impl->splines.end()) return {};
    return (index<(uint32_t)it->second.points.size())?it->second.points[index]:SplinePoint{};
}
uint32_t SplineSystem::PointCount(uint32_t id) const{
    auto it=m_impl->splines.find(id); return it!=m_impl->splines.end()?(uint32_t)it->second.points.size():0;
}
void SplineSystem::SetClosed(uint32_t id, bool c){
    auto it=m_impl->splines.find(id); if(it!=m_impl->splines.end()){ it->second.info.closed=c; it->second.BuildArcTable(); }
}
void SplineSystem::RebuildCache(uint32_t id){
    auto it=m_impl->splines.find(id); if(it!=m_impl->splines.end()) it->second.BuildArcTable();
}

void SplineSystem::Evaluate(uint32_t id, float t,
                              float pos[3], float tang[3], float norm[3], float binom[3]) const {
    auto it=m_impl->splines.find(id); if(it==m_impl->splines.end()){ for(int i=0;i<3;++i){ pos[i]=0; if(tang) tang[i]=0; } return; }
    it->second.EvalCatmullRom(t,pos,tang);
    if(tang && norm){
        // compute normal via cross with world-up
        float up[3]={0,1,0};
        norm[0]=up[1]*tang[2]-up[2]*tang[1];
        norm[1]=up[2]*tang[0]-up[0]*tang[2];
        norm[2]=up[0]*tang[1]-up[1]*tang[0];
        float l=std::sqrt(norm[0]*norm[0]+norm[1]*norm[1]+norm[2]*norm[2]);
        if(l>1e-6f){ norm[0]/=l;norm[1]/=l;norm[2]/=l; }
        if(binom){
            binom[0]=tang[1]*norm[2]-tang[2]*norm[1];
            binom[1]=tang[2]*norm[0]-tang[0]*norm[2];
            binom[2]=tang[0]*norm[1]-tang[1]*norm[0];
        }
    }
}

float SplineSystem::TotalLength(uint32_t id) const{
    auto it=m_impl->splines.find(id); return it!=m_impl->splines.end()?it->second.totalLen:0.f;
}
float SplineSystem::DistanceToT(uint32_t id, float dist) const{
    auto it=m_impl->splines.find(id); return it!=m_impl->splines.end()?it->second.DistanceToT(dist):0.f;
}
float SplineSystem::NearestT(uint32_t id, const float wp[3]) const{
    auto it=m_impl->splines.find(id); if(it==m_impl->splines.end()) return 0.f;
    int steps=100; float bestT=0.f, bestDist=1e30f;
    for(int i=0;i<=steps;++i){
        float t=(float)i/(float)steps;
        float pos[3]; it->second.EvalCatmullRom(t,pos,nullptr);
        float dx=pos[0]-wp[0],dy=pos[1]-wp[1],dz=pos[2]-wp[2];
        float d=dx*dx+dy*dy+dz*dz;
        if(d<bestDist){ bestDist=d; bestT=t; }
    }
    return bestT;
}
bool SplineSystem::SaveToFile(const std::string& path) const{
    std::ofstream f(path); if(!f.is_open()) return false;
    f<<"{\"count\":"<<m_impl->splines.size()<<"}\n"; return true;
}
bool SplineSystem::LoadFromFile(const std::string& path){
    std::ifstream f(path); return f.is_open();
}

} // namespace Engine
