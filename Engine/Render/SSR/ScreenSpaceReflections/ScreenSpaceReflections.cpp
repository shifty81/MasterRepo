#include "Engine/Render/SSR/ScreenSpaceReflections/ScreenSpaceReflections.h"
#include <algorithm>
#include <cmath>

namespace Engine {

static inline float Lerp(float a, float b, float t){ return a+(b-a)*t; }
static inline float Clamp01(float v){ return v<0?0:v>1?1:v; }

struct ScreenSpaceReflections::Impl {
    uint32_t maxSteps{64};
    float    stepSize{0.02f};
    float    maxDist {20.f};
    float    edgeFade{0.05f};
    float    roughCut{0.8f};
    bool     jitter  {true};

    float fallR{0.1f}, fallG{0.15f}, fallB{0.25f};

    // Projection params
    float fovY{1.047f}, aspect{1.778f}, nearZ{0.1f}, farZ{1000.f};
    Mat4  view{};

    // Project view-space point to NDC [0,1]
    bool ProjectVS(Vec3 p, uint32_t /*w*/, uint32_t /*h*/, Vec2& uv, float& depth) const {
        float tanHalfY = std::tan(fovY*0.5f);
        float tanHalfX = tanHalfY*aspect;
        if(p.z>=0) return false; // behind camera
        float nx = p.x / (-p.z * tanHalfX);
        float ny = p.y / (-p.z * tanHalfY);
        uv.x = nx*0.5f+0.5f;
        uv.y = ny*0.5f+0.5f;
        depth = (-p.z - nearZ)/(farZ - nearZ);
        return uv.x>=0&&uv.x<=1&&uv.y>=0&&uv.y<=1&&depth>=0&&depth<=1;
    }

    float SampleDepth(const float* buf, uint32_t w, uint32_t h, Vec2 uv) const {
        int px=(int)(uv.x*w), py=(int)(uv.y*h);
        px=std::max(0,std::min((int)w-1,px));
        py=std::max(0,std::min((int)h-1,py));
        return buf[py*w+px];
    }

    float EdgeFadeWeight(Vec2 uv) const {
        float fx=Clamp01(std::min(uv.x, 1.f-uv.x)/edgeFade);
        float fy=Clamp01(std::min(uv.y, 1.f-uv.y)/edgeFade);
        return fx*fy;
    }
};

ScreenSpaceReflections::ScreenSpaceReflections(): m_impl(new Impl){}
ScreenSpaceReflections::~ScreenSpaceReflections(){ Shutdown(); delete m_impl; }

void ScreenSpaceReflections::Init(uint32_t maxSteps, float stepSize){
    m_impl->maxSteps = maxSteps;
    m_impl->stepSize = stepSize;
}
void ScreenSpaceReflections::Shutdown(){}
void ScreenSpaceReflections::Reset(){ *m_impl = Impl(); }

void ScreenSpaceReflections::SetProjection(float fovY, float aspect, float nearZ, float farZ){
    m_impl->fovY=fovY; m_impl->aspect=aspect; m_impl->nearZ=nearZ; m_impl->farZ=farZ;
}
void ScreenSpaceReflections::SetViewMatrix(const Mat4& v){ m_impl->view=v; }

SSRHit ScreenSpaceReflections::RayMarch(Vec3 o, Vec3 d,
                                         const float* depth, const float* /*nrm*/,
                                         uint32_t w, uint32_t h) const {
    SSRHit hit{};
    // Normalise direction
    float len=std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z);
    if(len<1e-6f) return hit;
    d.x/=len; d.y/=len; d.z/=len;

    float jitterOffset = m_impl->jitter ? 0.3f : 0.f; // fixed blue-noise stub
    Vec3 p{o.x+d.x*jitterOffset*m_impl->stepSize,
           o.y+d.y*jitterOffset*m_impl->stepSize,
           o.z+d.z*jitterOffset*m_impl->stepSize};

    for(uint32_t i=0;i<m_impl->maxSteps;i++){
        p.x+=d.x*m_impl->stepSize; p.y+=d.y*m_impl->stepSize; p.z+=d.z*m_impl->stepSize;
        float dist=std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z);
        if(dist>m_impl->maxDist) break;

        Vec2 uv; float rayDepth;
        if(!m_impl->ProjectVS(p,w,h,uv,rayDepth)) continue;
        float sceneDepth = m_impl->SampleDepth(depth,w,h,uv);
        if(rayDepth>sceneDepth+1e-4f && rayDepth<sceneDepth+0.1f){
            hit.valid      = true;
            hit.uv         = uv;
            hit.confidence = m_impl->EdgeFadeWeight(uv) * Clamp01(1.f-(float)i/m_impl->maxSteps);
            return hit;
        }
    }
    return hit; // miss → caller uses fallback
}

void ScreenSpaceReflections::Blit(const float* col, const float* refl,
                                   const float* rough, float* out,
                                   uint32_t w, uint32_t h) const {
    for(uint32_t i=0;i<w*h;i++){
        float r = rough[i];
        float weight = Clamp01(1.f - r/m_impl->roughCut);
        float rc=refl[i*4+0], gc=refl[i*4+1], bc=refl[i*4+2];
        // blend original + reflection
        out[i*4+0] = Lerp(col[i*4+0], rc, weight);
        out[i*4+1] = Lerp(col[i*4+1], gc, weight);
        out[i*4+2] = Lerp(col[i*4+2], bc, weight);
        out[i*4+3] = col[i*4+3];
    }
}

void ScreenSpaceReflections::SetMaxSteps   (uint32_t s){ m_impl->maxSteps=s; }
void ScreenSpaceReflections::SetStepSize   (float s)   { m_impl->stepSize=s; }
void ScreenSpaceReflections::SetMaxDistance(float d)   { m_impl->maxDist=d; }
void ScreenSpaceReflections::SetEdgeFade   (float m)   { m_impl->edgeFade=m; }
void ScreenSpaceReflections::SetRoughnessCutoff(float r){ m_impl->roughCut=r; }
void ScreenSpaceReflections::SetJitter     (bool e)    { m_impl->jitter=e; }
void ScreenSpaceReflections::SetFallbackColour(float r,float g,float b){ m_impl->fallR=r; m_impl->fallG=g; m_impl->fallB=b; }
uint32_t ScreenSpaceReflections::MaxSteps() const { return m_impl->maxSteps; }
float    ScreenSpaceReflections::StepSize() const { return m_impl->stepSize; }

} // namespace Engine
