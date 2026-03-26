#include "Engine/Sim/FluidSim/FluidSimulation.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <vector>

namespace Engine {

// ── Grid indexing ─────────────────────────────────────────────────────────────

static inline size_t Idx(uint32_t x,uint32_t y,uint32_t z,
                          uint32_t W,uint32_t H){
    return (size_t)z*W*H + (size_t)y*W + x;
}

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct FluidSimulation::Impl {
    FluidSimConfig     cfg;
    uint32_t           W{0},H{0},D{0};
    size_t             N{0};

    std::vector<float> density;
    std::vector<float> densityPrev;
    std::vector<float> u;          ///< x velocity
    std::vector<float> v;          ///< y velocity
    std::vector<float> w;          ///< z velocity
    std::vector<float> uPrev;
    std::vector<float> vPrev;
    std::vector<float> wPrev;
    std::vector<bool>  solid;

    FluidVelocityField velField;
    uint32_t stepIdx{0};
    std::function<void(uint32_t)> onStep;

    void Alloc(){
        N=W*H*D;
        density.assign(N,0.f); densityPrev.assign(N,0.f);
        u.assign(N,0.f); v.assign(N,0.f); w.assign(N,0.f);
        uPrev.assign(N,0.f); vPrev.assign(N,0.f); wPrev.assign(N,0.f);
        solid.assign(N,false);
    }

    // Semi-Lagrangian advection (density or velocity component)
    void Advect(std::vector<float>& d, const std::vector<float>& d0,
                const std::vector<float>& uu, const std::vector<float>& vv,
                const std::vector<float>& ww, float dt){
        for(uint32_t k=0;k<D;++k) for(uint32_t j=0;j<H;++j) for(uint32_t i=0;i<W;++i){
            size_t idx=Idx(i,j,k,W,H);
            float px = (float)i - dt*uu[idx];
            float py = (float)j - dt*vv[idx];
            float pz = (float)k - dt*ww[idx];
            px=std::max(0.f,std::min((float)(W-1),px));
            py=std::max(0.f,std::min((float)(H-1),py));
            pz=std::max(0.f,std::min((float)(D-1),pz));
            int ix=(int)px, iy=(int)py, iz=(int)pz;
            float fx=px-ix, fy=py-iy, fz=pz-iz;
            ix=std::min(ix,(int)W-2); iy=std::min(iy,(int)H-2); iz=std::min(iz,(int)D-2);
            // Trilinear interpolation
            d[idx]=
                (1-fz)*((1-fy)*((1-fx)*d0[Idx(ix,iy,iz,W,H)]   +fx*d0[Idx(ix+1,iy,iz,W,H)])+
                               fy*((1-fx)*d0[Idx(ix,iy+1,iz,W,H)]+fx*d0[Idx(ix+1,iy+1,iz,W,H)]))+
                fz    *((1-fy)*((1-fx)*d0[Idx(ix,iy,iz+1,W,H)]+fx*d0[Idx(ix+1,iy,iz+1,W,H)])+
                               fy*((1-fx)*d0[Idx(ix,iy+1,iz+1,W,H)]+fx*d0[Idx(ix+1,iy+1,iz+1,W,H)]));
        }
    }

    // Gauss-Seidel pressure projection (simplified)
    void Project(float dt){
        (void)dt;
        std::vector<float> div(N,0.f), p(N,0.f);
        for(uint32_t k=1;k+1<D;++k) for(uint32_t j=1;j+1<H;++j) for(uint32_t i=1;i+1<W;++i){
            size_t idx=Idx(i,j,k,W,H);
            div[idx]=-0.5f*(u[Idx(i+1,j,k,W,H)]-u[Idx(i-1,j,k,W,H)]
                           +v[Idx(i,j+1,k,W,H)]-v[Idx(i,j-1,k,W,H)]
                           +w[Idx(i,j,k+1,W,H)]-w[Idx(i,j,k-1,W,H)]);
        }
        for(int iter=0;iter<(int)cfg.pressureIterations;++iter)
            for(uint32_t k=1;k+1<D;++k) for(uint32_t j=1;j+1<H;++j) for(uint32_t i=1;i+1<W;++i){
                size_t idx=Idx(i,j,k,W,H);
                p[idx]=(div[idx]+p[Idx(i-1,j,k,W,H)]+p[Idx(i+1,j,k,W,H)]
                                +p[Idx(i,j-1,k,W,H)]+p[Idx(i,j+1,k,W,H)]
                                +p[Idx(i,j,k-1,W,H)]+p[Idx(i,j,k+1,W,H)])/6.f;
            }
        for(uint32_t k=1;k+1<D;++k) for(uint32_t j=1;j+1<H;++j) for(uint32_t i=1;i+1<W;++i){
            u[Idx(i,j,k,W,H)]-=0.5f*(p[Idx(i+1,j,k,W,H)]-p[Idx(i-1,j,k,W,H)]);
            v[Idx(i,j,k,W,H)]-=0.5f*(p[Idx(i,j+1,k,W,H)]-p[Idx(i,j-1,k,W,H)]);
            w[Idx(i,j,k,W,H)]-=0.5f*(p[Idx(i,j,k+1,W,H)]-p[Idx(i,j,k-1,W,H)]);
        }
    }
};

FluidSimulation::FluidSimulation() : m_impl(new Impl()) {}
FluidSimulation::~FluidSimulation() { delete m_impl; }

void FluidSimulation::Init(const FluidSimConfig& cfg) {
    m_impl->cfg = cfg;
    m_impl->W = cfg.gridW; m_impl->H = cfg.gridH; m_impl->D = std::max(1u,cfg.gridD);
    m_impl->Alloc();
}

void FluidSimulation::Shutdown() { Reset(); }

void FluidSimulation::Reset() {
    m_impl->stepIdx = 0;
    std::fill(m_impl->density.begin(),m_impl->density.end(),0.f);
    std::fill(m_impl->u.begin(),m_impl->u.end(),0.f);
    std::fill(m_impl->v.begin(),m_impl->v.end(),0.f);
    std::fill(m_impl->w.begin(),m_impl->w.end(),0.f);
}

void FluidSimulation::AddDensitySource(uint32_t x,uint32_t y,uint32_t z,float amt) {
    if(x<m_impl->W&&y<m_impl->H&&z<m_impl->D)
        m_impl->density[Idx(x,y,z,m_impl->W,m_impl->H)] += amt;
}

void FluidSimulation::AddVelocitySource(uint32_t x,uint32_t y,uint32_t z,
                                         float vx,float vy,float vz) {
    if(x<m_impl->W&&y<m_impl->H&&z<m_impl->D){
        size_t idx=Idx(x,y,z,m_impl->W,m_impl->H);
        m_impl->u[idx]+=vx; m_impl->v[idx]+=vy; m_impl->w[idx]+=vz;
    }
}

void FluidSimulation::SetSolidCell(uint32_t x,uint32_t y,uint32_t z,bool s) {
    if(x<m_impl->W&&y<m_impl->H&&z<m_impl->D)
        m_impl->solid[Idx(x,y,z,m_impl->W,m_impl->H)]=s;
}

void FluidSimulation::SetGravity(float gx,float gy,float gz) {
    m_impl->cfg.gravity[0]=gx; m_impl->cfg.gravity[1]=gy; m_impl->cfg.gravity[2]=gz;
}

void FluidSimulation::AddGlobalForce(float fx,float fy,float fz) {
    for(auto& val:m_impl->u) val+=fx;
    for(auto& val:m_impl->v) val+=fy;
    for(auto& val:m_impl->w) val+=fz;
}

void FluidSimulation::Step(float dt) {
    // Advect velocity
    m_impl->uPrev=m_impl->u; m_impl->vPrev=m_impl->v; m_impl->wPrev=m_impl->w;
    m_impl->Advect(m_impl->u,m_impl->uPrev,m_impl->uPrev,m_impl->vPrev,m_impl->wPrev,dt);
    m_impl->Advect(m_impl->v,m_impl->vPrev,m_impl->uPrev,m_impl->vPrev,m_impl->wPrev,dt);
    if(m_impl->D>1)
        m_impl->Advect(m_impl->w,m_impl->wPrev,m_impl->uPrev,m_impl->vPrev,m_impl->wPrev,dt);
    // Apply gravity
    float gScale = dt;
    for(size_t i=0;i<m_impl->N;++i){
        m_impl->u[i]+=m_impl->cfg.gravity[0]*gScale;
        m_impl->v[i]+=m_impl->cfg.gravity[1]*gScale;
        m_impl->w[i]+=m_impl->cfg.gravity[2]*gScale;
    }
    m_impl->Project(dt);
    // Advect density
    m_impl->densityPrev=m_impl->density;
    m_impl->Advect(m_impl->density,m_impl->densityPrev,m_impl->u,m_impl->v,m_impl->w,dt);
    // Dissipate
    float diss = 1.f - m_impl->cfg.diffusion * dt;
    for(auto& d:m_impl->density) d=std::max(0.f,d*diss);
    ++m_impl->stepIdx;
    if(m_impl->onStep) m_impl->onStep(m_impl->stepIdx);
}

const std::vector<float>& FluidSimulation::GetDensityGrid() const { return m_impl->density; }

const FluidVelocityField& FluidSimulation::GetVelocityField() const {
    m_impl->velField.w=m_impl->W; m_impl->velField.h=m_impl->H; m_impl->velField.d=m_impl->D;
    m_impl->velField.u=m_impl->u; m_impl->velField.v=m_impl->v; m_impl->velField.w_vel=m_impl->w;
    return m_impl->velField;
}

FluidSimConfig FluidSimulation::GetConfig() const { return m_impl->cfg; }

float FluidSimulation::SampleDensity(uint32_t x,uint32_t y,uint32_t z) const {
    if(x<m_impl->W&&y<m_impl->H&&z<m_impl->D)
        return m_impl->density[Idx(x,y,z,m_impl->W,m_impl->H)];
    return 0.f;
}

void FluidSimulation::SampleVelocity(uint32_t x,uint32_t y,uint32_t z,
                                      float& vx,float& vy,float& vz) const {
    if(x<m_impl->W&&y<m_impl->H&&z<m_impl->D){
        size_t idx=Idx(x,y,z,m_impl->W,m_impl->H);
        vx=m_impl->u[idx]; vy=m_impl->v[idx]; vz=m_impl->w[idx];
    } else { vx=vy=vz=0.f; }
}

bool FluidSimulation::ExportDensityPGM(const std::string& path) const {
    std::ofstream f(path,std::ios::binary); if(!f.is_open()) return false;
    auto& d=m_impl->density;
    f<<"P5\n"<<m_impl->W<<" "<<m_impl->H<<"\n255\n";
    for(size_t i=0;i<(size_t)m_impl->W*m_impl->H;++i){
        uint8_t v=static_cast<uint8_t>(std::min(d[i],1.f)*255.f);
        f.write(reinterpret_cast<char*>(&v),1);
    }
    return true;
}

void FluidSimulation::OnStepComplete(std::function<void(uint32_t)> cb) {
    m_impl->onStep=std::move(cb);
}

} // namespace Engine
