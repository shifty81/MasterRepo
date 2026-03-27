#include "Engine/Render/SSAO/ScreenSpaceAO/ScreenSpaceAO.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

// Hemisphere sampling kernel (cosine-weighted)
static void BuildKernel(uint32_t n, std::vector<float>& out){
    out.resize(n*3);
    for(uint32_t i=0;i<n;i++){
        // Fibonacci hemisphere
        float phi=2.f*3.14159f*i/n;
        float cosTheta=std::sqrt((float)(i+1)/n);
        float sinTheta=std::sqrt(1-cosTheta*cosTheta);
        out[i*3+0]=std::cos(phi)*sinTheta;
        out[i*3+1]=cosTheta;
        out[i*3+2]=std::sin(phi)*sinTheta;
    }
}

static float Lerp(float a,float b,float t){ return a+(b-a)*t; }

struct ScreenSpaceAO::Impl {
    uint32_t w{256}, h{256};
    float    radius{0.5f};
    float    bias  {0.025f};
    float    intensity{1.f};
    uint32_t sampleCount{16};
    uint32_t blurRadius{2};
    bool     enabled{true};
    std::vector<float> noise;
    uint32_t noiseW{4}, noiseH{4};
    std::vector<float> kernel;
};

ScreenSpaceAO::ScreenSpaceAO(): m_impl(new Impl){ BuildKernel(16,m_impl->kernel); }
ScreenSpaceAO::~ScreenSpaceAO(){ Shutdown(); delete m_impl; }
void ScreenSpaceAO::Init(){}
void ScreenSpaceAO::Shutdown(){}
void ScreenSpaceAO::Reset(){ *m_impl=Impl{}; BuildKernel(16,m_impl->kernel); }

void ScreenSpaceAO::SetResolution (uint32_t w,uint32_t h){ m_impl->w=w; m_impl->h=h; }
void ScreenSpaceAO::SetRadius     (float r){ m_impl->radius=r; }
void ScreenSpaceAO::SetBias       (float b){ m_impl->bias=b; }
void ScreenSpaceAO::SetIntensity  (float i){ m_impl->intensity=i; }
void ScreenSpaceAO::SetSampleCount(uint32_t n){
    m_impl->sampleCount=std::min(64u,std::max(4u,n));
    BuildKernel(m_impl->sampleCount, m_impl->kernel);
}
void ScreenSpaceAO::SetBlurRadius (uint32_t px){ m_impl->blurRadius=px; }
void ScreenSpaceAO::SetEnabled    (bool on){ m_impl->enabled=on; }
bool ScreenSpaceAO::IsEnabled     () const { return m_impl->enabled; }

void ScreenSpaceAO::SetNoiseTexture(const float* data,uint32_t w,uint32_t h){
    m_impl->noiseW=w; m_impl->noiseH=h;
    m_impl->noise.assign(data,data+w*h*3);
}

bool ScreenSpaceAO::ComputeAO(const float* depthBuf, const float* normalBuf,
                                const SSAOMat4&, const SSAOMat4&,
                                std::vector<float>& outAO) const {
    if(!m_impl->enabled){ outAO.assign(m_impl->w*m_impl->h,1.f); return true; }
    outAO.resize(m_impl->w*m_impl->h);
    uint32_t n=m_impl->sampleCount;
    float r=m_impl->radius, bias=m_impl->bias;
    for(uint32_t py=0;py<m_impl->h;py++) for(uint32_t px=0;px<m_impl->w;px++){
        uint32_t pi=py*m_impl->w+px;
        float depth=depthBuf[pi];
        if(depth>=1.f){ outAO[pi]=1.f; continue; }
        // View-space normal (simplified: use normal buffer XYZ)
        float nx=normalBuf[pi*3+0], ny=normalBuf[pi*3+1], nz=normalBuf[pi*3+2];
        float occlusion=0;
        for(uint32_t s=0;s<n;s++){
            float sx=m_impl->kernel[s*3+0], sy=m_impl->kernel[s*3+1], sz=m_impl->kernel[s*3+2];
            // Flip sample if behind surface normal
            if(sx*nx+sy*ny+sz*nz<0){ sx=-sx; sy=-sy; sz=-sz; }
            // Project sample to screen (simplified: offset by screen radius)
            int spx=(int)px+(int)(sx*r*m_impl->w*0.5f);
            int spy=(int)py+(int)(sy*r*m_impl->h*0.5f);
            spx=std::max(0,std::min((int)m_impl->w-1,spx));
            spy=std::max(0,std::min((int)m_impl->h-1,spy));
            float sampleDepth=depthBuf[spy*m_impl->w+spx];
            float rangeCheck=std::abs(depth-sampleDepth)<r?1.f:0.f;
            occlusion+=(sampleDepth<=depth-bias?1.f:0.f)*rangeCheck;
        }
        outAO[pi]=std::pow(1.f-occlusion/n, m_impl->intensity);
    }
    return true;
}

void ScreenSpaceAO::Blur(const std::vector<float>& inAO,
                          std::vector<float>& outAO,
                          const float* depthBuf) const {
    outAO.resize(m_impl->w*m_impl->h);
    int br=(int)m_impl->blurRadius;
    for(uint32_t py=0;py<m_impl->h;py++) for(uint32_t px=0;px<m_impl->w;px++){
        float sum=0; float wSum=0;
        float cd=depthBuf[py*m_impl->w+px];
        for(int ky=-br;ky<=br;ky++) for(int kx=-br;kx<=br;kx++){
            int nx=(int)px+kx, ny=(int)py+ky;
            if(nx<0||ny<0||nx>=(int)m_impl->w||ny>=(int)m_impl->h) continue;
            uint32_t ni=ny*m_impl->w+nx;
            float nd=depthBuf[ni];
            float w=std::exp(-std::abs(cd-nd)*10.f);
            sum+=inAO[ni]*w; wSum+=w;
        }
        outAO[py*m_impl->w+px]=(wSum>0?sum/wSum:inAO[py*m_impl->w+px]);
    }
}

void ScreenSpaceAO::Composite(const std::vector<float>& inColor,
                                const std::vector<float>& inAO,
                                std::vector<float>& outColor) const {
    uint32_t pixels=m_impl->w*m_impl->h;
    outColor.resize(pixels*4);
    for(uint32_t i=0;i<pixels;i++){
        float ao=inAO.size()>i?inAO[i]:1.f;
        outColor[i*4+0]=inColor.size()>i*4+0?inColor[i*4+0]*ao:0;
        outColor[i*4+1]=inColor.size()>i*4+1?inColor[i*4+1]*ao:0;
        outColor[i*4+2]=inColor.size()>i*4+2?inColor[i*4+2]*ao:0;
        outColor[i*4+3]=inColor.size()>i*4+3?inColor[i*4+3]:1;
    }
}

} // namespace Engine
