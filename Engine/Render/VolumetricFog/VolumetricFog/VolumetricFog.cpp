#include "Engine/Render/VolumetricFog/VolumetricFog/VolumetricFog.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

static inline float Clamp01f(float v){ return v<0?0:v>1?1:v; }

struct VolumetricFog::Impl {
    uint32_t vw{0},vh{0},vd{0};
    float worldScale{1.f};
    float globalDensity{0.f};
    std::vector<float> density;

    float sr{0.8f}, sg{0.85f}, sb{0.9f}; // scatter colour
    float absorption{0.5f};
    FogVec3 lightDir{0.5f,-1.f,0.3f};
    float phaseG{0.f};
    float time{0.f};

    int Idx(uint32_t x, uint32_t y, uint32_t z) const {
        return (int)((z*vh+y)*vw+x);
    }
    float SampleDensity(float wx, float wy, float wz) const {
        float gx=wx/worldScale*(vw-1);
        float gy=wy/worldScale*(vh-1);
        float gz=wz/worldScale*(vd-1);
        int ix=(int)gx, iy=(int)gy, iz=(int)gz;
        ix=std::max(0,std::min((int)vw-1,ix));
        iy=std::max(0,std::min((int)vh-1,iy));
        iz=std::max(0,std::min((int)vd-1,iz));
        return globalDensity + density[Idx((uint32_t)ix,(uint32_t)iy,(uint32_t)iz)];
    }
    // Henyey-Greenstein phase
    float Phase(float cosTheta) const {
        float g=phaseG, g2=g*g;
        float denom=1.f+g2-2.f*g*cosTheta;
        return (1.f-g2)/(4.f*3.14159265f*denom*std::sqrt(denom)+1e-6f);
    }
};

VolumetricFog::VolumetricFog(): m_impl(new Impl){}
VolumetricFog::~VolumetricFog(){ Shutdown(); delete m_impl; }

void VolumetricFog::Init(uint32_t vw, uint32_t vh, uint32_t vd, float ws){
    m_impl->vw=vw; m_impl->vh=vh; m_impl->vd=vd; m_impl->worldScale=ws;
    m_impl->density.assign(vw*vh*vd, 0.f);
}
void VolumetricFog::Shutdown(){ m_impl->density.clear(); }
void VolumetricFog::Reset(){ std::fill(m_impl->density.begin(),m_impl->density.end(),0.f); m_impl->time=0.f; }
void VolumetricFog::Tick(float dt){ m_impl->time+=dt; }

void VolumetricFog::SetDensity(uint32_t x,uint32_t y,uint32_t z,float d){
    if(x<m_impl->vw&&y<m_impl->vh&&z<m_impl->vd)
        m_impl->density[m_impl->Idx(x,y,z)]=d;
}
float VolumetricFog::GetDensity(uint32_t x,uint32_t y,uint32_t z) const {
    if(x<m_impl->vw&&y<m_impl->vh&&z<m_impl->vd)
        return m_impl->density[m_impl->Idx(x,y,z)];
    return 0.f;
}

void VolumetricFog::FillBox(float mnX,float mnY,float mnZ,float mxX,float mxY,float mxZ,float d){
    float ws=m_impl->worldScale;
    int x0=(int)(mnX/ws*(m_impl->vw-1)), x1=(int)(mxX/ws*(m_impl->vw-1));
    int y0=(int)(mnY/ws*(m_impl->vh-1)), y1=(int)(mxY/ws*(m_impl->vh-1));
    int z0=(int)(mnZ/ws*(m_impl->vd-1)), z1=(int)(mxZ/ws*(m_impl->vd-1));
    x0=std::max(0,x0); x1=std::min((int)m_impl->vw-1,x1);
    y0=std::max(0,y0); y1=std::min((int)m_impl->vh-1,y1);
    z0=std::max(0,z0); z1=std::min((int)m_impl->vd-1,z1);
    for(int z=z0;z<=z1;z++) for(int y=y0;y<=y1;y++) for(int x=x0;x<=x1;x++)
        m_impl->density[m_impl->Idx((uint32_t)x,(uint32_t)y,(uint32_t)z)]=d;
}
void VolumetricFog::SetGlobalDensity(float d){ m_impl->globalDensity=d; }

FogSample VolumetricFog::RaymarchFog(FogVec3 o, FogVec3 dir, float tMin, float tMax, uint32_t steps) const {
    FogSample out{0,0,0,0,0};
    float step=(tMax-tMin)/std::max(1u,steps);
    float lLen=std::sqrt(m_impl->lightDir.x*m_impl->lightDir.x+m_impl->lightDir.y*m_impl->lightDir.y+m_impl->lightDir.z*m_impl->lightDir.z);
    float dLen=std::sqrt(dir.x*dir.x+dir.y*dir.y+dir.z*dir.z);
    float cosTheta=(lLen>1e-6f&&dLen>1e-6f)?
        (dir.x*m_impl->lightDir.x+dir.y*m_impl->lightDir.y+dir.z*m_impl->lightDir.z)/(dLen*lLen):0.f;
    float phase=m_impl->Phase(cosTheta);
    float transmit=1.f;
    for(uint32_t i=0;i<steps;i++){
        float t=tMin+(i+0.5f)*step;
        float px=o.x+dir.x*t, py=o.y+dir.y*t, pz=o.z+dir.z*t;
        float dens=m_impl->SampleDensity(px,py,pz);
        float ext=dens*m_impl->absorption;
        float scat=dens*(1.f-m_impl->absorption);
        float dT=std::exp(-ext*step);
        float inScatter=scat*phase*step;
        out.r+=transmit*m_impl->sr*inScatter;
        out.g+=transmit*m_impl->sg*inScatter;
        out.b+=transmit*m_impl->sb*inScatter;
        transmit*=dT;
        if(transmit<0.001f) break;
    }
    out.alpha=Clamp01f(1.f-transmit);
    out.depth=(tMax+tMin)*0.5f;
    return out;
}

void VolumetricFog::SetScatterColour(float r,float g,float b){ m_impl->sr=r; m_impl->sg=g; m_impl->sb=b; }
void VolumetricFog::SetAbsorption   (float a){ m_impl->absorption=a; }
void VolumetricFog::SetLightDir     (float x,float y,float z){ m_impl->lightDir={x,y,z}; }
void VolumetricFog::SetPhaseG       (float g){ m_impl->phaseG=g; }

void VolumetricFog::AnimateDensity(float wx, float wy, float wz, float dt){
    // Simple shift by wind*dt cells
    int sx=(int)(wx*dt), sy=(int)(wy*dt), sz=(int)(wz*dt);
    if(sx==0&&sy==0&&sz==0) return;
    std::vector<float> tmp=m_impl->density;
    for(int z=0;z<(int)m_impl->vd;z++) for(int y=0;y<(int)m_impl->vh;y++) for(int x=0;x<(int)m_impl->vw;x++){
        int nx=(x-sx+(int)m_impl->vw)%(int)m_impl->vw;
        int ny=(y-sy+(int)m_impl->vh)%(int)m_impl->vh;
        int nz=(z-sz+(int)m_impl->vd)%(int)m_impl->vd;
        m_impl->density[m_impl->Idx((uint32_t)x,(uint32_t)y,(uint32_t)z)]=tmp[m_impl->Idx((uint32_t)nx,(uint32_t)ny,(uint32_t)nz)];
    }
}

uint32_t VolumetricFog::VolumeW() const { return m_impl->vw; }
uint32_t VolumetricFog::VolumeH() const { return m_impl->vh; }
uint32_t VolumetricFog::VolumeD() const { return m_impl->vd; }

} // namespace Engine
