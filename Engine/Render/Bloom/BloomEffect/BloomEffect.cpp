#include "Engine/Render/Bloom/BloomEffect/BloomEffect.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

static void GaussianBlur1D(const float* src, float* dst, uint32_t w, uint32_t h,
                            bool horizontal, uint32_t radius){
    std::vector<float> kernel(2*radius+1);
    float sigma=radius*0.5f+0.5f, sum=0;
    for(uint32_t i=0;i<kernel.size();i++){
        float x=(int32_t)i-(int32_t)radius;
        kernel[i]=std::exp(-x*x/(2*sigma*sigma));
        sum+=kernel[i];
    }
    for(auto& k:kernel) k/=sum;

    for(uint32_t y=0;y<h;y++){
        for(uint32_t x=0;x<w;x++){
            float rr=0,gg=0,bb=0,aa=0;
            for(int32_t k=-(int32_t)radius;k<=(int32_t)radius;k++){
                uint32_t sx,sy;
                if(horizontal){
                    sx=(uint32_t)std::max(0,std::min((int32_t)w-1,(int32_t)x+k));
                    sy=y;
                } else {
                    sx=x;
                    sy=(uint32_t)std::max(0,std::min((int32_t)h-1,(int32_t)y+k));
                }
                float w2=kernel[k+radius];
                uint32_t idx=(sy*w+sx)*4;
                rr+=src[idx+0]*w2; gg+=src[idx+1]*w2;
                bb+=src[idx+2]*w2; aa+=src[idx+3]*w2;
            }
            uint32_t oidx=(y*w+x)*4;
            dst[oidx+0]=rr; dst[oidx+1]=gg; dst[oidx+2]=bb; dst[oidx+3]=aa;
        }
    }
}

struct BloomEffect::Impl {
    float    threshold{0.8f};
    float    intensity{1.f};
    uint32_t blurRadius{4};
    uint32_t mipLevels{4};
    float    dirtIntensity{0};
    bool     kawaseMode{false};
    uint32_t w{0}, h{0};
    std::vector<float> brightPass;
    std::vector<float> output;
    std::vector<float> dirtMask;
    uint32_t dirtW{0}, dirtH{0};
};

BloomEffect::BloomEffect(): m_impl(new Impl){}
BloomEffect::~BloomEffect(){ Shutdown(); delete m_impl; }
void BloomEffect::Init(){}
void BloomEffect::Shutdown(){ m_impl->brightPass.clear(); m_impl->output.clear(); }
void BloomEffect::Reset(){ *m_impl=Impl{}; }

void BloomEffect::SetThreshold   (float t){ m_impl->threshold=t; }
void BloomEffect::SetIntensity   (float k){ m_impl->intensity=k; }
void BloomEffect::SetBlurRadius  (uint32_t r){ m_impl->blurRadius=r; }
void BloomEffect::SetMipLevels   (uint32_t n){ m_impl->mipLevels=n; }
void BloomEffect::SetDirtIntensity(float k){ m_impl->dirtIntensity=k; }
void BloomEffect::EnableKawaseMode(bool on){ m_impl->kawaseMode=on; }
void BloomEffect::SetDirtMask(const float* mask, uint32_t w, uint32_t h){
    m_impl->dirtW=w; m_impl->dirtH=h;
    m_impl->dirtMask.assign(mask,mask+w*h*4);
}

void BloomEffect::Process(const float* src, uint32_t w, uint32_t h){
    m_impl->w=w; m_impl->h=h;
    uint32_t px=w*h*4;
    m_impl->brightPass.resize(px);
    m_impl->output.resize(px);

    // Bright pass (threshold extraction, luminance-based)
    for(uint32_t i=0;i<w*h;i++){
        float r=src[i*4+0], g=src[i*4+1], b=src[i*4+2], a=src[i*4+3];
        float lum=0.2126f*r+0.7152f*g+0.0722f*b;
        float scale=std::max(0.f,lum-m_impl->threshold)/(lum+1e-6f);
        m_impl->brightPass[i*4+0]=r*scale;
        m_impl->brightPass[i*4+1]=g*scale;
        m_impl->brightPass[i*4+2]=b*scale;
        m_impl->brightPass[i*4+3]=a;
    }

    // Blur bright pass (downsample chain simplified to single pass)
    std::vector<float> tmp(px);
    uint32_t r=m_impl->blurRadius;
    // Horizontal
    GaussianBlur1D(m_impl->brightPass.data(), tmp.data(), w, h, true, r);
    // Vertical
    GaussianBlur1D(tmp.data(), m_impl->brightPass.data(), w, h, false, r);

    // Blend bloom onto source
    float ki=m_impl->intensity;
    for(uint32_t i=0;i<w*h;i++){
        float bloom_r=m_impl->brightPass[i*4+0];
        float bloom_g=m_impl->brightPass[i*4+1];
        float bloom_b=m_impl->brightPass[i*4+2];

        // Apply dirt mask if set
        if(!m_impl->dirtMask.empty() && m_impl->dirtIntensity>0){
            uint32_t dx=(i%w)*m_impl->dirtW/std::max(1u,w);
            uint32_t dy=(i/w)*m_impl->dirtH/std::max(1u,h);
            uint32_t didx=(dy*m_impl->dirtW+dx)*4;
            if(didx+2<m_impl->dirtMask.size()){
                float dm=(m_impl->dirtMask[didx]+m_impl->dirtMask[didx+1]+
                          m_impl->dirtMask[didx+2])/3.f;
                bloom_r+=bloom_r*dm*m_impl->dirtIntensity;
                bloom_g+=bloom_g*dm*m_impl->dirtIntensity;
                bloom_b+=bloom_b*dm*m_impl->dirtIntensity;
            }
        }

        m_impl->output[i*4+0]=src[i*4+0]+bloom_r*ki;
        m_impl->output[i*4+1]=src[i*4+1]+bloom_g*ki;
        m_impl->output[i*4+2]=src[i*4+2]+bloom_b*ki;
        m_impl->output[i*4+3]=src[i*4+3];
    }
}

const float* BloomEffect::GetLastBrightPassPixels() const {
    return m_impl->brightPass.empty()?nullptr:m_impl->brightPass.data();
}
const float* BloomEffect::GetOutputPixels() const {
    return m_impl->output.empty()?nullptr:m_impl->output.data();
}
uint32_t BloomEffect::GetWidth () const { return m_impl->w; }
uint32_t BloomEffect::GetHeight() const { return m_impl->h; }

} // namespace Engine
