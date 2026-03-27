#include "Engine/Render/MotionBlur/MotionBlur/MotionBlur.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

// Minimal 4×4 matrix×vec4 transform
static void TransformVec4(const MBMat4& m, float x, float y, float z, float w,
                           float& ox, float& oy, float& oz, float& ow){
    ox=m.m[0]*x+m.m[4]*y+m.m[8]*z +m.m[12]*w;
    oy=m.m[1]*x+m.m[5]*y+m.m[9]*z +m.m[13]*w;
    oz=m.m[2]*x+m.m[6]*y+m.m[10]*z+m.m[14]*w;
    ow=m.m[3]*x+m.m[7]*y+m.m[11]*z+m.m[15]*w;
}

struct ObjMatrices { MBMat4 prev; MBMat4 curr; };

struct MotionBlur::Impl {
    float    shutterAngle{180.f};
    uint32_t maxSamples  {8};
    float    maxBlurPx   {32.f};
    float    camBlend    {0.5f};
    bool     enabled     {true};
    uint32_t width{0}, height{0};
    std::unordered_map<uint32_t,ObjMatrices> objects;
    std::function<void(uint32_t,uint32_t)> onApply;
};

MotionBlur::MotionBlur(): m_impl(new Impl){}
MotionBlur::~MotionBlur(){ Shutdown(); delete m_impl; }
void MotionBlur::Init(){}
void MotionBlur::Shutdown(){}
void MotionBlur::Clear(){ m_impl->objects.clear(); }

void MotionBlur::SetShutterAngle (float d){ m_impl->shutterAngle=d; }
void MotionBlur::SetMaxSamples   (uint32_t n){ m_impl->maxSamples=std::max(1u,n); }
void MotionBlur::SetMaxBlurRadius(float px){ m_impl->maxBlurPx=px; }
void MotionBlur::SetCameraBlend  (float v){ m_impl->camBlend=v; }
void MotionBlur::SetEnabled      (bool on){ m_impl->enabled=on; }
bool MotionBlur::IsEnabled       () const { return m_impl->enabled; }

void MotionBlur::RegisterObject  (uint32_t id, const MBMat4& prev, const MBMat4& curr){
    m_impl->objects[id]={prev,curr};
}
void MotionBlur::UnregisterObject(uint32_t id){ m_impl->objects.erase(id); }

void MotionBlur::ComputeVelocityBuffer(uint32_t w, uint32_t h,
                                        std::vector<float>& outVel) const {
    outVel.assign(w*h*2, 0.f); // RG = velocity
    // For each pixel, compute velocity from NDC prev/curr reprojection (stub)
    float shutFrac=m_impl->shutterAngle/360.f;
    for(uint32_t py=0;py<h;py++) for(uint32_t px=0;px<w;px++){
        float ndcX=2.f*px/w-1.f, ndcY=2.f*py/h-1.f;
        // Simple shutter-based velocity jitter per pixel
        float vx=ndcX*shutFrac*0.01f, vy=ndcY*shutFrac*0.01f;
        outVel[(py*w+px)*2+0]=vx;
        outVel[(py*w+px)*2+1]=vy;
    }
    (void)w; (void)h;
}

void MotionBlur::Apply(uint32_t w, uint32_t h,
                        const std::vector<float>& inColor,
                        const std::vector<float>& inVel,
                        std::vector<float>& outColor) const {
    if(m_impl->onApply) m_impl->onApply(w,h);
    outColor.resize(w*h*4);
    if(!m_impl->enabled||inColor.size()<w*h*4||inVel.size()<w*h*2){
        outColor=inColor; return;
    }
    uint32_t samps=m_impl->maxSamples;
    float maxPx=m_impl->maxBlurPx;
    for(uint32_t py=0;py<h;py++) for(uint32_t px=0;px<w;px++){
        uint32_t pi=py*w+px;
        float vx=inVel[pi*2+0]*w, vy=inVel[pi*2+1]*h;
        float len=std::sqrt(vx*vx+vy*vy);
        if(len>maxPx){ float sc=maxPx/len; vx*=sc; vy*=sc; }
        float r=0,g=0,b=0,a=0;
        for(uint32_t s=0;s<samps;s++){
            float t=(samps>1)?(float)s/(samps-1):0;
            float sx=px+vx*(t-0.5f), sy=py+vy*(t-0.5f);
            int ix=(int)std::max(0.f,std::min((float)(w-1),sx));
            int iy=(int)std::max(0.f,std::min((float)(h-1),sy));
            uint32_t si=(iy*w+ix)*4;
            r+=inColor[si]; g+=inColor[si+1]; b+=inColor[si+2]; a+=inColor[si+3];
        }
        float inv=1.f/samps;
        outColor[pi*4+0]=r*inv; outColor[pi*4+1]=g*inv;
        outColor[pi*4+2]=b*inv; outColor[pi*4+3]=a*inv;
    }
}
void MotionBlur::SetOnApply(std::function<void(uint32_t,uint32_t)> cb){ m_impl->onApply=cb; }

} // namespace Engine
