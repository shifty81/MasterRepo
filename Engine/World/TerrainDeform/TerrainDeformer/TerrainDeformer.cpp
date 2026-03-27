#include "Engine/World/TerrainDeform/TerrainDeformer/TerrainDeformer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace Engine {

struct TerrainDeformer::Impl {
    uint32_t         w{0}, d{0};
    float            cellSize{1.f};
    std::vector<float> heights;
    uint32_t         dirtyMinX{UINT32_MAX}, dirtyMinZ{UINT32_MAX};
    uint32_t         dirtyMaxX{0},         dirtyMaxZ{0};
    bool             dirty{false};

    uint32_t Idx(uint32_t x, uint32_t z) const { return z*w+x; }
    bool InBounds(uint32_t x, uint32_t z) const { return x<w&&z<d; }

    void WorldToCell(float wx, float wz, int32_t& cx, int32_t& cz) const {
        cx=(int32_t)std::floor(wx/cellSize);
        cz=(int32_t)std::floor(wz/cellSize);
    }

    void MarkDirty(uint32_t x, uint32_t z){
        dirty=true;
        if(x<dirtyMinX) dirtyMinX=x;
        if(x>dirtyMaxX) dirtyMaxX=x;
        if(z<dirtyMinZ) dirtyMinZ=z;
        if(z>dirtyMaxZ) dirtyMaxZ=z;
    }

    void BrushKernel(float wx, float wz, float radius, float strength, float dt,
                     bool raise, bool smooth, bool flatten, float flatTarget){
        int32_t cx,cz; WorldToCell(wx,wz,cx,cz);
        int32_t rCells=(int32_t)std::ceil(radius/cellSize)+1;
        for(int32_t iz=cz-rCells;iz<=cz+rCells;iz++){
            for(int32_t ix=cx-rCells;ix<=cx+rCells;ix++){
                if(!InBounds((uint32_t)std::max(0,ix),(uint32_t)std::max(0,iz))) continue;
                if(ix<0||iz<0) continue;
                uint32_t ux=(uint32_t)ix, uz=(uint32_t)iz;
                if(!InBounds(ux,uz)) continue;
                float dx=(ix-cx)*cellSize, dz=(iz-cz)*cellSize;
                float dist=std::sqrt(dx*dx+dz*dz);
                if(dist>radius) continue;
                float falloff=1.f-dist/radius;
                float delta=strength*falloff*dt;
                if(smooth){
                    // Average with neighbours
                    float sum=heights[Idx(ux,uz)]; int cnt=1;
                    if(ux>0){sum+=heights[Idx(ux-1,uz)];cnt++;}
                    if(ux+1<w){sum+=heights[Idx(ux+1,uz)];cnt++;}
                    if(uz>0){sum+=heights[Idx(ux,uz-1)];cnt++;}
                    if(uz+1<d){sum+=heights[Idx(ux,uz+1)];cnt++;}
                    float avg=sum/cnt;
                    heights[Idx(ux,uz)]+=delta*(avg-heights[Idx(ux,uz)]);
                } else if(flatten){
                    heights[Idx(ux,uz)]+=(flatTarget-heights[Idx(ux,uz)])*delta;
                } else {
                    heights[Idx(ux,uz)]+=raise?delta:-delta;
                }
                MarkDirty(ux,uz);
            }
        }
    }
};

TerrainDeformer::TerrainDeformer(): m_impl(new Impl){}
TerrainDeformer::~TerrainDeformer(){ Shutdown(); delete m_impl; }

void TerrainDeformer::Init(uint32_t w, uint32_t d, float cs){
    m_impl->w=w; m_impl->d=d; m_impl->cellSize=cs;
    m_impl->heights.assign(w*d, 0.f);
    ClearDirty();
}
void TerrainDeformer::Shutdown(){ m_impl->heights.clear(); }
void TerrainDeformer::Reset(){ m_impl->heights.assign(m_impl->w*m_impl->d,0.f); ClearDirty(); }

void  TerrainDeformer::SetHeight(uint32_t x, uint32_t z, float h){
    if(!m_impl->InBounds(x,z)) return;
    m_impl->heights[m_impl->Idx(x,z)]=h; m_impl->MarkDirty(x,z);
}
float TerrainDeformer::GetHeight(uint32_t x, uint32_t z) const {
    if(!m_impl->InBounds(x,z)) return 0;
    return m_impl->heights[m_impl->Idx(x,z)];
}

void TerrainDeformer::RaiseBrush  (float wx,float wz,float r,float s,float dt){
    m_impl->BrushKernel(wx,wz,r,s,dt,true ,false,false,0);
}
void TerrainDeformer::LowerBrush  (float wx,float wz,float r,float s,float dt){
    m_impl->BrushKernel(wx,wz,r,s,dt,false,false,false,0);
}
void TerrainDeformer::SmoothBrush (float wx,float wz,float r,float s,float dt){
    m_impl->BrushKernel(wx,wz,r,s,dt,false,true ,false,0);
}
void TerrainDeformer::FlattenBrush(float wx,float wz,float r,float t,float s,float dt){
    m_impl->BrushKernel(wx,wz,r,s,dt,false,false,true,t);
}

float TerrainDeformer::GetHeightAt(float wx, float wz) const {
    float cx=wx/m_impl->cellSize, cz=wz/m_impl->cellSize;
    int32_t x0=(int32_t)cx, z0=(int32_t)cz;
    float fx=cx-x0, fz=cz-z0;
    auto H=[&](int32_t x,int32_t z)->float{
        x=std::max(0,std::min((int32_t)m_impl->w-1,x));
        z=std::max(0,std::min((int32_t)m_impl->d-1,z));
        return m_impl->heights[m_impl->Idx((uint32_t)x,(uint32_t)z)];
    };
    return H(x0,z0)*(1-fx)*(1-fz)+H(x0+1,z0)*fx*(1-fz)
          +H(x0,z0+1)*(1-fx)*fz +H(x0+1,z0+1)*fx*fz;
}

TDVec3 TerrainDeformer::GetNormalAt(float wx, float wz) const {
    float cs=m_impl->cellSize;
    float hR=GetHeightAt(wx+cs,wz), hL=GetHeightAt(wx-cs,wz);
    float hU=GetHeightAt(wx,wz+cs), hD=GetHeightAt(wx,wz-cs);
    TDVec3 n{hL-hR, 2.f*cs, hD-hU};
    float len=std::sqrt(n.x*n.x+n.y*n.y+n.z*n.z);
    if(len>0){ n.x/=len; n.y/=len; n.z/=len; }
    return n;
}

void TerrainDeformer::ApplyErosion(uint32_t iters, float rain, float evap){
    std::vector<float> water(m_impl->w*m_impl->d, 0.f);
    auto H=[&](uint32_t x,uint32_t z)->float&{ return m_impl->heights[m_impl->Idx(x,z)]; };
    auto W=[&](uint32_t x,uint32_t z)->float&{ return water[m_impl->Idx(x,z)]; };
    for(uint32_t iter=0;iter<iters;iter++){
        for(uint32_t z=0;z<m_impl->d;z++) for(uint32_t x=0;x<m_impl->w;x++) W(x,z)+=rain;
        // Flow to lowest neighbour
        for(uint32_t z=1;z+1<m_impl->d;z++) for(uint32_t x=1;x+1<m_impl->w;x++){
            uint32_t bx=x,bz=z;
            auto check=[&](uint32_t nx,uint32_t nz){ if(H(nx,nz)<H(bx,bz)){bx=nx;bz=nz;} };
            check(x-1,z); check(x+1,z); check(x,z-1); check(x,z+1);
            if(bx!=x||bz!=z){
                float flow=W(x,z)*0.5f;
                W(x,z)-=flow; W(bx,bz)+=flow;
                float sediment=flow*0.01f;
                H(x,z)-=sediment; H(bx,bz)+=sediment;
            }
        }
        for(uint32_t z=0;z<m_impl->d;z++) for(uint32_t x=0;x<m_impl->w;x++)
            W(x,z)*=(1.f-evap);
    }
    // Mark all dirty
    for(uint32_t z=0;z<m_impl->d;z++) for(uint32_t x=0;x<m_impl->w;x++) m_impl->MarkDirty(x,z);
}

void TerrainDeformer::GetDirtyRegion(uint32_t& mnX,uint32_t& mnZ,uint32_t& mxX,uint32_t& mxZ) const {
    mnX=m_impl->dirtyMinX; mnZ=m_impl->dirtyMinZ;
    mxX=m_impl->dirtyMaxX; mxZ=m_impl->dirtyMaxZ;
}
void TerrainDeformer::ClearDirty(){
    m_impl->dirty=false;
    m_impl->dirtyMinX=UINT32_MAX; m_impl->dirtyMinZ=UINT32_MAX;
    m_impl->dirtyMaxX=0; m_impl->dirtyMaxZ=0;
}
bool TerrainDeformer::IsDirty() const { return m_impl->dirty; }

bool TerrainDeformer::SaveHeightmap(const std::string& path) const {
    FILE* f=fopen(path.c_str(),"wb"); if(!f) return false;
    fwrite(&m_impl->w,4,1,f); fwrite(&m_impl->d,4,1,f);
    fwrite(m_impl->heights.data(),4,m_impl->heights.size(),f);
    fclose(f); return true;
}
bool TerrainDeformer::LoadHeightmap(const std::string& path){
    FILE* f=fopen(path.c_str(),"rb"); if(!f) return false;
    uint32_t w,d;
    if(fread(&w,4,1,f)<1||fread(&d,4,1,f)<1){ fclose(f); return false; }
    Init(w,d,m_impl->cellSize);
    size_t n=fread(m_impl->heights.data(),4,w*d,f);
    fclose(f); return n==(size_t)(w*d);
}

uint32_t TerrainDeformer::GetWidth   () const { return m_impl->w; }
uint32_t TerrainDeformer::GetDepth   () const { return m_impl->d; }
float    TerrainDeformer::GetCellSize() const { return m_impl->cellSize; }

} // namespace Engine
