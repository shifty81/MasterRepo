#include "Engine/Render/ToneMapping/ToneMappingSystem/ToneMappingSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

struct ToneMappingSystem::Impl {
    TonemapOperator op{TonemapOperator::ACES};
    float exposure{0.f};
    float gamma   {2.2f};
    float white   {4.f};
    float saturation{1.f};
    float contrast  {1.f};
    std::function<void(float,float,float,float&,float&,float&)> customCurve;

    void ApplyGamma(float& r,float& g,float& b,float inv) const {
        r=std::pow(std::max(0.f,r),inv);
        g=std::pow(std::max(0.f,g),inv);
        b=std::pow(std::max(0.f,b),inv);
    }
    void ApplySaturation(float& r,float& g,float& b) const {
        float lum=0.2126f*r+0.7152f*g+0.0722f*b;
        r=lum+(r-lum)*saturation;
        g=lum+(g-lum)*saturation;
        b=lum+(b-lum)*saturation;
    }
    void ApplyContrast(float& r,float& g,float& b) const {
        auto sc=[&](float v){ return (v-0.5f)*contrast+0.5f; };
        r=std::max(0.f,std::min(1.f,sc(r)));
        g=std::max(0.f,std::min(1.f,sc(g)));
        b=std::max(0.f,std::min(1.f,sc(b)));
    }
    // ACES fitted (Krzysztof Narkowicz)
    float ACES_f(float x) const {
        const float a=2.51f,b=0.03f,c=2.43f,d=0.59f,e=0.14f;
        return std::max(0.f,std::min(1.f,(x*(a*x+b))/(x*(c*x+d)+e)));
    }
    // Uncharted2
    float UC2_f(float x) const {
        float A=0.22f,B=0.30f,C=0.10f,D=0.20f,E=0.01f,F=0.30f;
        return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
    }
};

ToneMappingSystem::ToneMappingSystem(): m_impl(new Impl){}
ToneMappingSystem::~ToneMappingSystem(){ Shutdown(); delete m_impl; }
void ToneMappingSystem::Init(){}
void ToneMappingSystem::Shutdown(){}
void ToneMappingSystem::Reset(){ *m_impl=Impl{}; }

void ToneMappingSystem::SetOperator  (TonemapOperator op){ m_impl->op=op; }
TonemapOperator ToneMappingSystem::GetOperator() const { return m_impl->op; }
void ToneMappingSystem::SetExposure  (float ev){ m_impl->exposure=ev; }
float ToneMappingSystem::GetExposure () const { return m_impl->exposure; }
void ToneMappingSystem::SetGamma     (float g){ m_impl->gamma=std::max(0.1f,g); }
float ToneMappingSystem::GetGamma    () const { return m_impl->gamma; }
void ToneMappingSystem::SetWhitePoint(float w){ m_impl->white=w; }
void ToneMappingSystem::SetSaturation(float s){ m_impl->saturation=s; }
void ToneMappingSystem::SetContrast  (float c){ m_impl->contrast=c; }

void ToneMappingSystem::ApplyPixel(float r,float g,float b,
                                    float& oR,float& oG,float& oB) const {
    // Exposure
    float scale=std::pow(2.f,m_impl->exposure);
    r*=scale; g*=scale; b*=scale;
    // Tone map
    switch(m_impl->op){
        case TonemapOperator::Linear:
            oR=std::min(1.f,r); oG=std::min(1.f,g); oB=std::min(1.f,b); break;
        case TonemapOperator::Reinhard:
            oR=r/(1+r); oG=g/(1+g); oB=b/(1+b); break;
        case TonemapOperator::ACES:
            oR=m_impl->ACES_f(r); oG=m_impl->ACES_f(g); oB=m_impl->ACES_f(b); break;
        case TonemapOperator::Uncharted2:{
            float W=m_impl->white;
            float norm=1.f/m_impl->UC2_f(W);
            oR=m_impl->UC2_f(r)*norm; oG=m_impl->UC2_f(g)*norm; oB=m_impl->UC2_f(b)*norm;
            break;}
        case TonemapOperator::Hejl:{
            auto hejl=[](float x){ float y=std::max(0.f,x-0.004f); return (y*(6.2f*y+0.5f))/(y*(6.2f*y+1.7f)+0.06f); };
            oR=hejl(r); oG=hejl(g); oB=hejl(b); break;}
        case TonemapOperator::Custom:
            if(m_impl->customCurve) m_impl->customCurve(r,g,b,oR,oG,oB);
            else { oR=r; oG=g; oB=b; } break;
        default: oR=r; oG=g; oB=b; break;
    }
    m_impl->ApplySaturation(oR,oG,oB);
    m_impl->ApplyContrast  (oR,oG,oB);
    m_impl->ApplyGamma     (oR,oG,oB, 1.f/m_impl->gamma);
}

void ToneMappingSystem::Apply(const std::vector<float>& in, std::vector<float>& out,
                               uint32_t pixels) const {
    out.resize(pixels*4);
    for(uint32_t i=0;i<pixels;i++){
        float r=in.size()>i*4?in[i*4]:0;
        float g=in.size()>i*4+1?in[i*4+1]:0;
        float b=in.size()>i*4+2?in[i*4+2]:0;
        float a=in.size()>i*4+3?in[i*4+3]:1;
        ApplyPixel(r,g,b,out[i*4],out[i*4+1],out[i*4+2]);
        out[i*4+3]=a;
    }
}

void ToneMappingSystem::SetCustomCurve(
    std::function<void(float,float,float,float&,float&,float&)> cb){
    m_impl->customCurve=cb;
}

void ToneMappingSystem::ComputeHistogram(const std::vector<float>& in, uint32_t pixels,
                                          std::vector<uint32_t>& bins, uint32_t binCount) const {
    bins.assign(binCount,0);
    for(uint32_t i=0;i<pixels;i++){
        float r=in.size()>i*4?in[i*4]:0;
        float g=in.size()>i*4+1?in[i*4+1]:0;
        float b=in.size()>i*4+2?in[i*4+2]:0;
        float lum=0.2126f*r+0.7152f*g+0.0722f*b;
        float logL=std::log2(std::max(1e-5f,lum)+1.f); // map [0,~16] → [0,~4]
        uint32_t bin=(uint32_t)(logL/4.f*(binCount-1));
        bin=std::min(bin,binCount-1);
        bins[bin]++;
    }
}

void ToneMappingSystem::AutoExposure(const std::vector<float>& in, uint32_t pixels){
    if(pixels==0) return;
    float avg=0;
    for(uint32_t i=0;i<pixels;i++){
        float r=in.size()>i*4?in[i*4]:0;
        float g=in.size()>i*4+1?in[i*4+1]:0;
        float b=in.size()>i*4+2?in[i*4+2]:0;
        avg+=0.2126f*r+0.7152f*g+0.0722f*b;
    }
    avg/=pixels;
    // Target 0.18 (middle grey)
    float target=0.18f;
    m_impl->exposure=std::log2(std::max(1e-5f,target/std::max(1e-5f,avg)));
}

} // namespace Engine
