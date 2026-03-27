#include "Engine/Spatial/SpatialHashGrid/SpatialHashGrid.h"
#include <algorithm>
#include <cmath>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace Engine {

struct ObjPos { float x,y,z; };

struct SHCell {
    int64_t key;
    std::vector<uint32_t> ids;
};

struct SpatialHashGrid::Impl {
    float cellSize{1.f};
    std::unordered_map<int64_t,std::vector<uint32_t>> grid;
    std::unordered_map<uint32_t,ObjPos>               positions;
    std::function<void(uint32_t)> onInsert;
    std::function<void(uint32_t)> onRemove;

    int64_t CellKey(int cx,int cy,int cz) const {
        // Pack 3 × 20 bits into 64-bit key (±512K cells per axis)
        int64_t x=(int64_t)(cx&0xFFFFF);
        int64_t y=(int64_t)(cy&0xFFFFF);
        int64_t z=(int64_t)(cz&0xFFFFF);
        return x|(y<<20)|(z<<40);
    }
    void CellOf(float wx,float wy,float wz,int& cx,int& cy,int& cz) const {
        cx=(int)std::floor(wx/cellSize);
        cy=(int)std::floor(wy/cellSize);
        cz=(int)std::floor(wz/cellSize);
    }
    void RemoveFromGrid(uint32_t id, float x,float y,float z){
        int cx,cy,cz; CellOf(x,y,z,cx,cy,cz);
        int64_t k=CellKey(cx,cy,cz);
        auto it=grid.find(k);
        if(it!=grid.end()){
            auto& v=it->second;
            v.erase(std::remove(v.begin(),v.end(),id),v.end());
            if(v.empty()) grid.erase(it);
        }
    }
    void AddToGrid(uint32_t id,float x,float y,float z){
        int cx,cy,cz; CellOf(x,y,z,cx,cy,cz);
        grid[CellKey(cx,cy,cz)].push_back(id);
    }
};

SpatialHashGrid::SpatialHashGrid(): m_impl(new Impl){}
SpatialHashGrid::~SpatialHashGrid(){ Shutdown(); delete m_impl; }
void SpatialHashGrid::Init(float cs){ m_impl->cellSize=cs; }
void SpatialHashGrid::Shutdown(){ Clear(); }
void SpatialHashGrid::Clear(){
    m_impl->grid.clear(); m_impl->positions.clear();
}
void SpatialHashGrid::SetCellSize(float s){ m_impl->cellSize=std::max(0.0001f,s); }

void SpatialHashGrid::Insert(uint32_t id, float x,float y,float z){
    m_impl->positions[id]={x,y,z};
    m_impl->AddToGrid(id,x,y,z);
    if(m_impl->onInsert) m_impl->onInsert(id);
}
void SpatialHashGrid::Update(uint32_t id,float x,float y,float z){
    auto it=m_impl->positions.find(id);
    if(it!=m_impl->positions.end())
        m_impl->RemoveFromGrid(id,it->second.x,it->second.y,it->second.z);
    m_impl->positions[id]={x,y,z};
    m_impl->AddToGrid(id,x,y,z);
}
void SpatialHashGrid::Remove(uint32_t id){
    auto it=m_impl->positions.find(id);
    if(it!=m_impl->positions.end()){
        m_impl->RemoveFromGrid(id,it->second.x,it->second.y,it->second.z);
        m_impl->positions.erase(it);
        if(m_impl->onRemove) m_impl->onRemove(id);
    }
}

uint32_t SpatialHashGrid::QueryRadius(float cx,float cy,float cz,float radius,
                                       std::vector<uint32_t>& out) const {
    out.clear();
    int minX,minY,minZ,maxX,maxY,maxZ;
    m_impl->CellOf(cx-radius,cy-radius,cz-radius,minX,minY,minZ);
    m_impl->CellOf(cx+radius,cy+radius,cz+radius,maxX,maxY,maxZ);
    float r2=radius*radius;
    for(int x=minX;x<=maxX;x++) for(int y=minY;y<=maxY;y++) for(int z=minZ;z<=maxZ;z++){
        auto it=m_impl->grid.find(m_impl->CellKey(x,y,z));
        if(it==m_impl->grid.end()) continue;
        for(uint32_t id:it->second){
            auto& p=m_impl->positions.at(id);
            float dx=p.x-cx,dy=p.y-cy,dz=p.z-cz;
            if(dx*dx+dy*dy+dz*dz<=r2) out.push_back(id);
        }
    }
    return (uint32_t)out.size();
}
uint32_t SpatialHashGrid::QueryBox(float mnX,float mnY,float mnZ,
                                    float mxX,float mxY,float mxZ,
                                    std::vector<uint32_t>& out) const {
    out.clear();
    int minX,minY,minZ,maxX,maxY,maxZ;
    m_impl->CellOf(mnX,mnY,mnZ,minX,minY,minZ);
    m_impl->CellOf(mxX,mxY,mxZ,maxX,maxY,maxZ);
    for(int x=minX;x<=maxX;x++) for(int y=minY;y<=maxY;y++) for(int z=minZ;z<=maxZ;z++){
        auto it=m_impl->grid.find(m_impl->CellKey(x,y,z));
        if(it==m_impl->grid.end()) continue;
        for(uint32_t id:it->second){
            auto& p=m_impl->positions.at(id);
            if(p.x>=mnX&&p.x<=mxX&&p.y>=mnY&&p.y<=mxY&&p.z>=mnZ&&p.z<=mxZ)
                out.push_back(id);
        }
    }
    return (uint32_t)out.size();
}
bool SpatialHashGrid::GetNearest(float cx,float cy,float cz,uint32_t& outId) const {
    if(m_impl->positions.empty()) return false;
    float bestD=1e30f; outId=0;
    for(auto& [id,p]:m_impl->positions){
        float d=(p.x-cx)*(p.x-cx)+(p.y-cy)*(p.y-cy)+(p.z-cz)*(p.z-cz);
        if(d<bestD){bestD=d;outId=id;}
    }
    return true;
}
uint32_t SpatialHashGrid::GetCount    () const { return (uint32_t)m_impl->positions.size(); }
uint32_t SpatialHashGrid::GetCellCount() const { return (uint32_t)m_impl->grid.size(); }

void SpatialHashGrid::SetOnInsert(std::function<void(uint32_t)> cb){ m_impl->onInsert=cb; }
void SpatialHashGrid::SetOnRemove(std::function<void(uint32_t)> cb){ m_impl->onRemove=cb; }

} // namespace Engine
