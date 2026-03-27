#include "Engine/Render/CSM/CascadedShadowMap/CascadedShadowMap.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace Engine {

static const float kPi=3.14159265f;

// Minimal 4x4 matrix math
static CSMMat4 Identity(){ CSMMat4 m{}; m.m[0]=m.m[5]=m.m[10]=m.m[15]=1; return m; }
static CSMVec3 Norm3(CSMVec3 v){
    float l=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
    return l>0?CSMVec3{v.x/l,v.y/l,v.z/l}:CSMVec3{0,1,0};
}
static float Dot3(CSMVec3 a,CSMVec3 b){ return a.x*b.x+a.y*b.y+a.z*b.z; }
static CSMVec3 Cross3(CSMVec3 a,CSMVec3 b){
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}

static CSMMat4 LookAt(CSMVec3 eye, CSMVec3 center, CSMVec3 up){
    CSMVec3 f=Norm3({center.x-eye.x,center.y-eye.y,center.z-eye.z});
    CSMVec3 r=Norm3(Cross3(f,up));
    CSMVec3 u=Cross3(r,f);
    CSMMat4 m=Identity();
    m.m[0]=r.x; m.m[4]=r.y; m.m[8]=r.z;  m.m[12]=-(r.x*eye.x+r.y*eye.y+r.z*eye.z);
    m.m[1]=u.x; m.m[5]=u.y; m.m[9]=u.z;  m.m[13]=-(u.x*eye.x+u.y*eye.y+u.z*eye.z);
    m.m[2]=-f.x;m.m[6]=-f.y;m.m[10]=-f.z;m.m[14]= (f.x*eye.x+f.y*eye.y+f.z*eye.z);
    m.m[15]=1;
    return m;
}
static CSMMat4 OrthoSymm(float w, float h, float zNear, float zFar){
    CSMMat4 m{};
    m.m[0]=2.f/w; m.m[5]=2.f/h; m.m[10]=-2.f/(zFar-zNear);
    m.m[14]=-(zFar+zNear)/(zFar-zNear); m.m[15]=1;
    return m;
}

struct CascadeData {
    float    splitDepth{0};
    float    depthBias {0};
    CSMMat4  viewMat;
    CSMMat4  projMat;
};

struct CascadedShadowMap::Impl {
    uint32_t  cascadeCount{4};
    uint32_t  mapW{1024}, mapH{1024};
    float     lambda{0.5f};
    uint32_t  pcfKernel{3};
    float     blendBand{0.5f};
    CSMVec3   lightDir{-0.577f,-0.577f,0.577f};
    // Camera
    float     fovY{60}, aspect{1.777f}, zNear{0.1f}, zFar{500.f};
    CSMVec3   camPos{0,0,0}, camFwd{0,0,-1}, camUp{0,1,0};

    std::array<CascadeData,kMaxCascades> cascades{};
};

CascadedShadowMap::CascadedShadowMap(): m_impl(new Impl){}
CascadedShadowMap::~CascadedShadowMap(){ Shutdown(); delete m_impl; }
void CascadedShadowMap::Init(){}
void CascadedShadowMap::Shutdown(){}
void CascadedShadowMap::Reset(){ *m_impl=Impl{}; }

void CascadedShadowMap::SetCascadeCount(uint32_t n){ m_impl->cascadeCount=std::min(n,kMaxCascades); }
void CascadedShadowMap::SetLightDir    (CSMVec3 d){ m_impl->lightDir=Norm3(d); }
void CascadedShadowMap::SetCameraFrustum(float fy,float asp,float zn,float zf,
                                          CSMVec3 cp,CSMVec3 cf,CSMVec3 cu){
    m_impl->fovY=fy; m_impl->aspect=asp; m_impl->zNear=zn; m_impl->zFar=zf;
    m_impl->camPos=cp; m_impl->camFwd=cf; m_impl->camUp=cu;
}
void CascadedShadowMap::SetShadowMapSize(uint32_t w,uint32_t h){ m_impl->mapW=w; m_impl->mapH=h; }
void CascadedShadowMap::SetLambda      (float l){ m_impl->lambda=l; }
void CascadedShadowMap::SetDepthBias   (uint32_t c,float b){ if(c<kMaxCascades) m_impl->cascades[c].depthBias=b; }
void CascadedShadowMap::SetPCFKernelSize(uint32_t n){ m_impl->pcfKernel=n; }
void CascadedShadowMap::SetBlendBand   (float w){ m_impl->blendBand=w; }

void CascadedShadowMap::ComputeSplits(){
    float n=m_impl->zNear, f=m_impl->zFar, lam=m_impl->lambda;
    uint32_t cnt=m_impl->cascadeCount;
    for(uint32_t i=0;i<cnt;i++){
        float iF=(float)(i+1)/cnt;
        float log=n*std::pow(f/n,iF);
        float uni=n+(f-n)*iF;
        m_impl->cascades[i].splitDepth=lam*log+(1-lam)*uni;
    }
}
void CascadedShadowMap::ComputeLightMatrices(){
    // Build per-cascade tight ortho frusta around camera sub-frustum
    float tanHalfFovY=std::tan(m_impl->fovY*0.5f*kPi/180.f);
    float tanHalfFovX=tanHalfFovY*m_impl->aspect;
    CSMVec3 ld=Norm3(m_impl->lightDir);
    CSMVec3 up={0,1,0};
    if(std::abs(Dot3(ld,up))>0.99f) up={1,0,0};

    float prevSplit=m_impl->zNear;
    for(uint32_t ci=0;ci<m_impl->cascadeCount;ci++){
        float farSplit=m_impl->cascades[ci].splitDepth;
        // Frustum corners for this sub-frustum (simplified AABB in light space)
        float hw=tanHalfFovX*farSplit, hh=tanHalfFovY*farSplit;
        float sphere=std::sqrt(hw*hw+hh*hh+farSplit*farSplit)*0.5f; // bounding sphere radius
        // Light view looks along -lightDir toward scene centre
        CSMVec3 centre={m_impl->camPos.x+m_impl->camFwd.x*farSplit*0.5f,
                        m_impl->camPos.y+m_impl->camFwd.y*farSplit*0.5f,
                        m_impl->camPos.z+m_impl->camFwd.z*farSplit*0.5f};
        CSMVec3 eye={centre.x-ld.x*sphere*2,centre.y-ld.y*sphere*2,centre.z-ld.z*sphere*2};
        m_impl->cascades[ci].viewMat=LookAt(eye,centre,up);
        m_impl->cascades[ci].projMat=OrthoSymm(sphere*2,sphere*2,0.1f,sphere*4+0.1f);
        prevSplit=farSplit;
    }
    (void)prevSplit;
}

CSMMat4  CascadedShadowMap::GetLightViewMatrix(uint32_t c) const { return c<kMaxCascades?m_impl->cascades[c].viewMat:Identity(); }
CSMMat4  CascadedShadowMap::GetLightProjMatrix(uint32_t c) const { return c<kMaxCascades?m_impl->cascades[c].projMat:Identity(); }
float    CascadedShadowMap::GetSplitDepth     (uint32_t c) const { return c<kMaxCascades?m_impl->cascades[c].splitDepth:0; }
uint32_t CascadedShadowMap::GetPCFKernelSize  () const { return m_impl->pcfKernel; }
uint32_t CascadedShadowMap::GetCascadeCount   () const { return m_impl->cascadeCount; }

float CascadedShadowMap::SampleShadow(CSMVec3 worldPos, uint32_t cascade) const {
    if(cascade>=m_impl->cascadeCount) return 0.f;
    // Transform worldPos to light clip space and return 1 (lit) as stub
    (void)worldPos;
    return 1.f;
}

} // namespace Engine
