#include "Engine/World/Terrain/TerrainSystem/TerrainSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct TerrainData {
    std::vector<float> hmap;
    uint32_t w{0}, h{0};
    float    scale{1.f};
    // per-layer splat maps (up to 4)
    std::vector<std::vector<float>> splats;
};

struct TerrainSystem::Impl {
    std::unordered_map<uint32_t,TerrainData> terrains;
    TerrainData* Find(uint32_t id){ auto it=terrains.find(id); return it!=terrains.end()?&it->second:nullptr; }

    // Bilinear sample
    float Sample(const TerrainData& t, float wx, float wz) const {
        if(t.w<2||t.h<2) return 0;
        float gx=wx/t.scale, gz=wz/t.scale;
        int ix=(int)gx, iz=(int)gz;
        float fx=gx-ix, fz=gz-iz;
        auto clamp=[&](int v, int maxV){ return std::max(0,std::min(v,maxV)); };
        int x0=clamp(ix,t.w-1),x1=clamp(ix+1,t.w-1);
        int z0=clamp(iz,t.h-1),z1=clamp(iz+1,t.h-1);
        float h00=t.hmap[z0*t.w+x0], h10=t.hmap[z0*t.w+x1];
        float h01=t.hmap[z1*t.w+x0], h11=t.hmap[z1*t.w+x1];
        return (h00*(1-fx)+h10*fx)*(1-fz)+(h01*(1-fx)+h11*fx)*fz;
    }
    void SetCell(TerrainData& t, int ix, int iz, float v){
        ix=std::max(0,std::min(ix,(int)t.w-1));
        iz=std::max(0,std::min(iz,(int)t.h-1));
        t.hmap[iz*t.w+ix]=std::max(0.f,std::min(1.f,v));
    }
};

TerrainSystem::TerrainSystem(): m_impl(new Impl){}
TerrainSystem::~TerrainSystem(){ Shutdown(); delete m_impl; }
void TerrainSystem::Init(){}
void TerrainSystem::Shutdown(){ m_impl->terrains.clear(); }
void TerrainSystem::Reset(){ m_impl->terrains.clear(); }

bool TerrainSystem::LoadHeightmap(uint32_t id, const std::vector<float>& data,
                                   uint32_t w, uint32_t h, float scale){
    if(data.size()<w*h) return false;
    TerrainData t; t.w=w; t.h=h; t.scale=scale;
    t.hmap.assign(data.begin(),data.begin()+w*h);
    m_impl->terrains[id]=std::move(t);
    return true;
}

float TerrainSystem::GetHeight(uint32_t id, float wx, float wz) const {
    auto* t=m_impl->Find(id); return t?m_impl->Sample(*t,wx,wz):0;
}
void TerrainSystem::GetNormal(uint32_t id, float wx, float wz,
                               float& nx, float& ny, float& nz) const {
    auto* t=m_impl->Find(id); if(!t){nx=0;ny=1;nz=0;return;}
    float eps=t->scale;
    float hL=m_impl->Sample(*t,wx-eps,wz);
    float hR=m_impl->Sample(*t,wx+eps,wz);
    float hD=m_impl->Sample(*t,wx,wz-eps);
    float hU=m_impl->Sample(*t,wx,wz+eps);
    nx=hL-hR; ny=2*eps; nz=hD-hU;
    float l=std::sqrt(nx*nx+ny*ny+nz*nz); if(l>0){nx/=l;ny/=l;nz/=l;}
}

void TerrainSystem::SetHeight(uint32_t id, float wx, float wz, float h){
    auto* t=m_impl->Find(id); if(!t) return;
    int ix=(int)(wx/t->scale), iz=(int)(wz/t->scale);
    m_impl->SetCell(*t,ix,iz,h);
}

void TerrainSystem::DeformSphere(uint32_t id, float cx, float cz,
                                  float radius, float strength){
    auto* t=m_impl->Find(id); if(!t) return;
    int minX=std::max(0,(int)((cx-radius)/t->scale));
    int maxX=std::min((int)t->w-1,(int)((cx+radius)/t->scale));
    int minZ=std::max(0,(int)((cz-radius)/t->scale));
    int maxZ=std::min((int)t->h-1,(int)((cz+radius)/t->scale));
    for(int iz=minZ;iz<=maxZ;iz++) for(int ix=minX;ix<=maxX;ix++){
        float wx=ix*t->scale, wz2=iz*t->scale;
        float d=std::sqrt((wx-cx)*(wx-cx)+(wz2-cz)*(wz2-cz));
        if(d>=radius) continue;
        float w=1.f-d/radius;
        float& cell=t->hmap[iz*t->w+ix];
        cell=std::max(0.f,std::min(1.f,cell+strength*w));
    }
}

void TerrainSystem::FlattenArea(uint32_t id, float cx, float cz,
                                 float radius, float targetH){
    auto* t=m_impl->Find(id); if(!t) return;
    int minX=std::max(0,(int)((cx-radius)/t->scale));
    int maxX=std::min((int)t->w-1,(int)((cx+radius)/t->scale));
    int minZ=std::max(0,(int)((cz-radius)/t->scale));
    int maxZ=std::min((int)t->h-1,(int)((cz+radius)/t->scale));
    for(int iz=minZ;iz<=maxZ;iz++) for(int ix=minX;ix<=maxX;ix++){
        float wx=ix*t->scale, wz2=iz*t->scale;
        float d=std::sqrt((wx-cx)*(wx-cx)+(wz2-cz)*(wz2-cz));
        if(d>=radius) continue;
        float blend=1.f-d/radius;
        float& cell=t->hmap[iz*t->w+ix];
        cell=cell*(1-blend)+targetH*blend;
    }
}

void TerrainSystem::SmoothArea(uint32_t id, float cx, float cz,
                                float radius, uint32_t iterations){
    auto* t=m_impl->Find(id); if(!t||t->w<3||t->h<3) return;
    for(uint32_t k=0;k<iterations;k++){
        std::vector<float> tmp=t->hmap;
        int minX=std::max(1,(int)((cx-radius)/t->scale));
        int maxX=std::min((int)t->w-2,(int)((cx+radius)/t->scale));
        int minZ=std::max(1,(int)((cz-radius)/t->scale));
        int maxZ=std::min((int)t->h-2,(int)((cz+radius)/t->scale));
        for(int iz=minZ;iz<=maxZ;iz++) for(int ix=minX;ix<=maxX;ix++){
            float avg=(t->hmap[(iz-1)*t->w+ix]+t->hmap[(iz+1)*t->w+ix]+
                       t->hmap[iz*t->w+ix-1]+t->hmap[iz*t->w+ix+1]+
                       t->hmap[iz*t->w+ix])/5.f;
            tmp[iz*t->w+ix]=avg;
        }
        t->hmap=tmp;
    }
}

void TerrainSystem::HydraulicErosion(uint32_t id, uint32_t iters, float rain){
    auto* t=m_impl->Find(id); if(!t) return;
    for(uint32_t k=0;k<iters;k++){
        // Simplified: deposit at local min, erode at local max
        for(uint32_t iz=1;iz+1<t->h;iz++) for(uint32_t ix=1;ix+1<t->w;ix++){
            float h=t->hmap[iz*t->w+ix];
            float mn=h;
            mn=std::min(mn,t->hmap[(iz-1)*t->w+ix]);
            mn=std::min(mn,t->hmap[(iz+1)*t->w+ix]);
            mn=std::min(mn,t->hmap[iz*t->w+ix-1]);
            mn=std::min(mn,t->hmap[iz*t->w+ix+1]);
            float erosion=std::min(rain,h-mn);
            t->hmap[iz*t->w+ix]-=erosion;
        }
    }
}

void TerrainSystem::SetSplatmap(uint32_t id, uint32_t layer, const std::vector<float>& w){
    auto* t=m_impl->Find(id); if(!t) return;
    if(layer>=t->splats.size()) t->splats.resize(layer+1);
    t->splats[layer]=w;
}
float TerrainSystem::GetSplatWeight(uint32_t id, uint32_t layer,
                                     float wx, float wz) const {
    auto* t=m_impl->Find(id); if(!t||layer>=t->splats.size()) return 0;
    int ix=(int)(wx/t->scale), iz=(int)(wz/t->scale);
    ix=std::max(0,std::min(ix,(int)t->w-1));
    iz=std::max(0,std::min(iz,(int)t->h-1));
    uint32_t idx=iz*t->w+ix;
    if(idx>=t->splats[layer].size()) return 0;
    return t->splats[layer][idx];
}

uint32_t TerrainSystem::GetMapWidth  (uint32_t id) const { auto* t=m_impl->Find(id);return t?t->w:0; }
uint32_t TerrainSystem::GetMapHeight (uint32_t id) const { auto* t=m_impl->Find(id);return t?t->h:0; }
float    TerrainSystem::GetWorldScale(uint32_t id) const { auto* t=m_impl->Find(id);return t?t->scale:1; }
uint32_t TerrainSystem::ExportHeightmap(uint32_t id, std::vector<float>& out) const {
    auto* t=m_impl->Find(id); if(!t) return 0;
    out=t->hmap; return (uint32_t)out.size();
}

} // namespace Engine
