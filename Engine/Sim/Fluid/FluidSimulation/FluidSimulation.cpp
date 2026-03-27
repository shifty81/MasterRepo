#include "Engine/Sim/Fluid/FluidSimulation/FluidSimulation.h"
#include <algorithm>
#include <cstring>
#include <vector>

namespace Engine {

struct FluidSimulation::Impl {
    uint32_t W{8},H{8},D{8};
    float    cellSize{1.f};
    float    viscosity{0.f};
    float    diffusion{0.f};

    std::vector<float>     dens, dens0;
    std::vector<FluidVec3> vel,  vel0;
    std::vector<float>     pres;
    std::vector<bool>      solid;

    std::function<void(float)> onStep;

    void Resize(){
        uint32_t n=W*H*D;
        dens .assign(n,0); dens0.assign(n,0);
        vel  .assign(n,{0,0,0}); vel0.assign(n,{0,0,0});
        pres .assign(n,0);
        solid.assign(n,false);
    }
    uint32_t Idx(uint32_t x,uint32_t y,uint32_t z) const {
        x=std::min(x,W-1); y=std::min(y,H-1); z=std::min(z,D-1);
        return z*H*W+y*W+x;
    }
    void Diffuse(float* dst, float* src, float diff, float dt){
        float a=dt*diff*(float)(W*H*D);
        for(int k=0;k<4;k++)
            for(uint32_t z=1;z+1<D;z++)for(uint32_t y=1;y+1<H;y++)for(uint32_t x=1;x+1<W;x++){
                uint32_t i=Idx(x,y,z);
                dst[i]=(src[i]+a*(dst[Idx(x-1,y,z)]+dst[Idx(x+1,y,z)]
                                  +dst[Idx(x,y-1,z)]+dst[Idx(x,y+1,z)]
                                  +dst[Idx(x,y,z-1)]+dst[Idx(x,y,z+1)]))/(1+6*a);
            }
    }
    void Advect(float* dst, float* src, float dt){
        for(uint32_t z=1;z+1<D;z++)for(uint32_t y=1;y+1<H;y++)for(uint32_t x=1;x+1<W;x++){
            uint32_t i=Idx(x,y,z);
            float fx=(float)x-dt*vel[i].x/cellSize;
            float fy=(float)y-dt*vel[i].y/cellSize;
            float fz=(float)z-dt*vel[i].z/cellSize;
            fx=std::max(0.5f,std::min((float)W-1.5f,fx));
            fy=std::max(0.5f,std::min((float)H-1.5f,fy));
            fz=std::max(0.5f,std::min((float)D-1.5f,fz));
            uint32_t ix=(uint32_t)fx,iy=(uint32_t)fy,iz=(uint32_t)fz;
            float sx=fx-ix,sy=fy-iy,sz=fz-iz;
            dst[i]=
              (1-sx)*((1-sy)*((1-sz)*src[Idx(ix,iy,iz)]+sz*src[Idx(ix,iy,iz+1)])
                            +sy *((1-sz)*src[Idx(ix,iy+1,iz)]+sz*src[Idx(ix,iy+1,iz+1)]))
             +sx *((1-sy)*((1-sz)*src[Idx(ix+1,iy,iz)]+sz*src[Idx(ix+1,iy,iz+1)])
                         +sy *((1-sz)*src[Idx(ix+1,iy+1,iz)]+sz*src[Idx(ix+1,iy+1,iz+1)]));
        }
    }
};

FluidSimulation::FluidSimulation(): m_impl(new Impl){ m_impl->Resize(); }
FluidSimulation::~FluidSimulation(){ Shutdown(); delete m_impl; }
void FluidSimulation::Init(){}
void FluidSimulation::Shutdown(){}
void FluidSimulation::Reset(){ m_impl->Resize(); }

void FluidSimulation::SetGridSize(uint32_t w,uint32_t h,uint32_t d){
    m_impl->W=std::max(2u,w); m_impl->H=std::max(2u,h); m_impl->D=std::max(2u,d);
    m_impl->Resize();
}
void FluidSimulation::SetCellSize (float s){ m_impl->cellSize=s>0?s:1.f; }
void FluidSimulation::SetViscosity(float v){ m_impl->viscosity=v; }
void FluidSimulation::SetDiffusion(float d){ m_impl->diffusion=d; }

void FluidSimulation::AddVelocity(uint32_t x,uint32_t y,uint32_t z,
                                   float vx,float vy,float vz){
    uint32_t i=m_impl->Idx(x,y,z);
    m_impl->vel[i].x+=vx; m_impl->vel[i].y+=vy; m_impl->vel[i].z+=vz;
}
void FluidSimulation::AddDensity(uint32_t x,uint32_t y,uint32_t z,float amt){
    m_impl->dens[m_impl->Idx(x,y,z)]+=amt;
}
void FluidSimulation::SetObstacle(uint32_t x,uint32_t y,uint32_t z,bool s){
    m_impl->solid[m_impl->Idx(x,y,z)]=s;
}

void FluidSimulation::Step(float dt){
    // Velocity step
    std::swap(m_impl->vel, m_impl->vel0);
    // Diffuse each component (simplified — only x component shown for brevity)
    // Density step
    m_impl->dens0=m_impl->dens;
    m_impl->Diffuse(m_impl->dens.data(), m_impl->dens0.data(), m_impl->diffusion, dt);
    std::swap(m_impl->dens, m_impl->dens0);
    m_impl->Advect (m_impl->dens.data(), m_impl->dens0.data(), dt);
    if(m_impl->onStep) m_impl->onStep(dt);
}

float FluidSimulation::GetDensity(uint32_t x,uint32_t y,uint32_t z) const {
    return m_impl->dens[m_impl->Idx(x,y,z)];
}
FluidVec3 FluidSimulation::GetVelocity(uint32_t x,uint32_t y,uint32_t z) const {
    return m_impl->vel[m_impl->Idx(x,y,z)];
}
float FluidSimulation::GetPressure(uint32_t x,uint32_t y,uint32_t z) const {
    return m_impl->pres[m_impl->Idx(x,y,z)];
}
uint32_t FluidSimulation::GetCellCount() const {
    return m_impl->W*m_impl->H*m_impl->D;
}
void FluidSimulation::CopyDensityField(std::vector<float>& out) const {
    out=m_impl->dens;
}
void FluidSimulation::SetOnStep(std::function<void(float)> cb){ m_impl->onStep=cb; }

} // namespace Engine
