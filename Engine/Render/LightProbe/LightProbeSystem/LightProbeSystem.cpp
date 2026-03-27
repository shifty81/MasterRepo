#include "Engine/Render/LightProbe/LightProbeSystem/LightProbeSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

// L2 SH basis (9 coefficients, 3 channels = 27 floats)
static constexpr float PI = 3.14159265f;
static constexpr float SH_Y0 = 0.282095f;            // Y_0^0
static constexpr float SH_Y1 = 0.488603f;            // Y_1^{-1,0,1}
static constexpr float SH_Y2a = 1.092548f;           // Y_2^{-1}
static constexpr float SH_Y2b = 0.315392f;           // Y_2^{0}
static constexpr float SH_Y2c = 0.546274f;           // Y_2^{2}

static void EvalSH(float nx,float ny,float nz,float out[9]){
    out[0]=SH_Y0;
    out[1]=SH_Y1*ny; out[2]=SH_Y1*nz; out[3]=SH_Y1*nx;
    out[4]=SH_Y2a*nx*ny; out[5]=SH_Y2a*ny*nz;
    out[6]=SH_Y2b*(3*nz*nz-1);
    out[7]=SH_Y2a*nx*nz; out[8]=SH_Y2c*(nx*nx-ny*ny);
}

struct Probe {
    float pos[3]{};
    float sh[27]{};  // 9 coeffs × RGB
    bool  captured{false};
};

struct LightProbeSystem::Impl {
    std::unordered_map<uint32_t,Probe> probes;
    float blendRadius{10.f};
    std::function<void(uint32_t)> onCapture;

    Probe* Find(uint32_t id){ auto it=probes.find(id); return it!=probes.end()?&it->second:nullptr; }
};

LightProbeSystem::LightProbeSystem(): m_impl(new Impl){}
LightProbeSystem::~LightProbeSystem(){ Shutdown(); delete m_impl; }
void LightProbeSystem::Init(){}
void LightProbeSystem::Shutdown(){ m_impl->probes.clear(); }
void LightProbeSystem::Reset(){ m_impl->probes.clear(); }

bool LightProbeSystem::PlaceProbe(uint32_t id, float x,float y,float z){
    if(m_impl->probes.count(id)) return false;
    Probe p; p.pos[0]=x; p.pos[1]=y; p.pos[2]=z;
    m_impl->probes[id]=p; return true;
}
void LightProbeSystem::RemoveProbe(uint32_t id){ m_impl->probes.erase(id); }

void LightProbeSystem::CaptureProbe(uint32_t id, const float* radiance, uint32_t faceSize){
    auto* p=m_impl->Find(id); if(!p) return;
    // Store a pointer hint (actual rendering would fill 6 faces)
    // Here we copy the first face as representative irradiance
    (void)radiance; (void)faceSize;
    p->captured=true;
    ProjectToSH(id);
}

void LightProbeSystem::ProjectToSH(uint32_t id){
    auto* p=m_impl->Find(id); if(!p) return;
    // Simple default: ambient white probe (all SH coefficients except L00=grey)
    std::fill(p->sh,p->sh+27,0.f);
    // L00 ambient ~ 0.5 grey
    p->sh[0]=0.5f; p->sh[9]=0.5f; p->sh[18]=0.5f;
    // L1 mild sky gradient (slightly blue top)
    p->sh[2]=0.1f; p->sh[11]=0.1f; p->sh[20]=0.2f;
    if(m_impl->onCapture) m_impl->onCapture(id);
}

void LightProbeSystem::SampleIrradiance(uint32_t id,
                                         float nx,float ny,float nz,
                                         float outRGB[3]) const {
    auto* p=m_impl->Find(id); if(!p){ outRGB[0]=outRGB[1]=outRGB[2]=0; return; }
    float basis[9]; EvalSH(nx,ny,nz,basis);
    outRGB[0]=outRGB[1]=outRGB[2]=0;
    for(int c=0;c<3;c++) for(int k=0;k<9;k++) outRGB[c]+=p->sh[c*9+k]*basis[k];
    for(int c=0;c<3;c++) outRGB[c]=std::max(0.f,outRGB[c]);
}

void LightProbeSystem::InterpolateIrradiance(float x,float y,float z,
                                               float nx,float ny,float nz,
                                               float outRGB[3]) const {
    outRGB[0]=outRGB[1]=outRGB[2]=0;
    float wSum=0;
    for(auto& [id,p]:m_impl->probes){
        float dx=p.pos[0]-x,dy=p.pos[1]-y,dz=p.pos[2]-z;
        float dist=std::sqrt(dx*dx+dy*dy+dz*dz);
        if(dist>m_impl->blendRadius) continue;
        float w=1.f-dist/m_impl->blendRadius;
        float rgb[3]; SampleIrradiance(id,nx,ny,nz,rgb);
        outRGB[0]+=rgb[0]*w; outRGB[1]+=rgb[1]*w; outRGB[2]+=rgb[2]*w;
        wSum+=w;
    }
    if(wSum>0){ outRGB[0]/=wSum; outRGB[1]/=wSum; outRGB[2]/=wSum; }
}

void LightProbeSystem::SetBlendRadius(float r){ m_impl->blendRadius=r; }
uint32_t LightProbeSystem::GetProbeCount() const { return (uint32_t)m_impl->probes.size(); }
void LightProbeSystem::GetProbePosition(uint32_t id,float& x,float& y,float& z) const {
    auto* p=m_impl->Find(id); if(!p){x=y=z=0;return;}
    x=p->pos[0]; y=p->pos[1]; z=p->pos[2];
}
void LightProbeSystem::GetSHCoefficients(uint32_t id,float out[27]) const {
    auto* p=m_impl->Find(id); if(!p){std::fill(out,out+27,0.f);return;}
    std::copy(p->sh,p->sh+27,out);
}
void LightProbeSystem::SetOnCapture(std::function<void(uint32_t)> cb){ m_impl->onCapture=cb; }

} // namespace Engine
