#include "Engine/Render/AtmosphericScattering/AtmosphericScattering/AtmosphericScattering.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

static float Dot3(const float a[3], const float b[3]){ return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
static float Len3(const float v[3]){ return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])+1e-9f; }

struct AtmosphericScattering::Impl {
    AtmosphericParams params;
    float sunDir[3]{0,1,0};
    bool  dirty{true};
    std::vector<float> lut; // w*h*3
    uint32_t lutW{0}, lutH{0};
    float timeHours{12.f};

    // Rayleigh phase function
    float RayleighPhase(float cosTheta) const {
        return (3.f/(16.f*3.14159f))*(1.f+cosTheta*cosTheta);
    }
    // Mie (Henyey-Greenstein)
    float MiePhase(float cosTheta) const {
        float g=params.mieG, g2=g*g;
        return (3.f/(8.f*3.14159f))*((1-g2)*(1+cosTheta*cosTheta))/
               ((2+g2)*std::pow(1+g2-2*g*cosTheta,1.5f));
    }
    // Integrate scattering along ray (simplified single scatter)
    void Integrate(const float viewDir[3], float outRGB[3]) const {
        const int nSteps=10;
        float rayLen=params.atmosRadius-params.earthRadius;
        float rayl[3]={};
        for(int s=0;s<nSteps;s++){
            float t=(s+0.5f)/nSteps;
            float h=params.rayleighScaleH*0.5f; // simplified flat layer
            float rScatter=std::exp(-t*rayLen/params.rayleighScaleH);
            float mScatter=std::exp(-t*rayLen/params.mieScaleH);
            float cosV=std::max(-1.f,std::min(1.f,Dot3(viewDir,sunDir)));
            float rPhase=RayleighPhase(cosV);
            float mPhase=MiePhase(cosV);
            for(int i=0;i<3;i++)
                rayl[i]+=params.rayleighCoeff[i]*rScatter*rPhase*params.sunIntensity/nSteps;
            float mContrib=params.mieCoeff*mScatter*mPhase*params.sunIntensity/nSteps;
            for(int i=0;i<3;i++) rayl[i]+=mContrib;
        }
        for(int i=0;i<3;i++) outRGB[i]=rayl[i];
    }
};

AtmosphericScattering::AtmosphericScattering()  : m_impl(new Impl){}
AtmosphericScattering::~AtmosphericScattering() { Shutdown(); delete m_impl; }
void AtmosphericScattering::Init()     {}
void AtmosphericScattering::Shutdown() { m_impl->lut.clear(); }

void AtmosphericScattering::SetParams(const AtmosphericParams& p){ m_impl->params=p; m_impl->dirty=true; }
const AtmosphericParams& AtmosphericScattering::GetParams() const { return m_impl->params; }

void AtmosphericScattering::SetSunDirection(const float d[3]){
    float l=Len3(d); for(int i=0;i<3;i++) m_impl->sunDir[i]=d[i]/l; m_impl->dirty=true;
}
void AtmosphericScattering::GetSunDirection(float out[3]) const {
    for(int i=0;i<3;i++) out[i]=m_impl->sunDir[i];
}
void AtmosphericScattering::SetTime(float h){
    m_impl->timeHours=h;
    float angle=(h-12.f)/12.f*3.14159f;
    m_impl->sunDir[0]=std::cos(angle); m_impl->sunDir[1]=std::sin(angle); m_impl->sunDir[2]=0;
    m_impl->dirty=true;
}

void AtmosphericScattering::BuildLUT(uint32_t w, uint32_t h){
    m_impl->lutW=w; m_impl->lutH=h;
    m_impl->lut.resize(w*h*3);
    for(uint32_t y=0;y<h;y++){
        float cosZenith=1.f-2.f*y/(float)(h-1);
        float sinZenith=std::sqrt(std::max(0.f,1.f-cosZenith*cosZenith));
        for(uint32_t x=0;x<w;x++){
            float cosSun=1.f-2.f*x/(float)(w-1);
            float sinSun=std::sqrt(std::max(0.f,1.f-cosSun*cosSun));
            float vd[3]={sinZenith,cosZenith,0};
            float sd[3]={sinSun,cosSun,0};
            float old[3]={m_impl->sunDir[0],m_impl->sunDir[1],m_impl->sunDir[2]};
            for(int i=0;i<3;i++) m_impl->sunDir[i]=sd[i];
            float rgb[3]; m_impl->Integrate(vd, rgb);
            for(int i=0;i<3;i++) m_impl->sunDir[i]=old[i];
            uint32_t idx=(y*w+x)*3;
            for(int i=0;i<3;i++) m_impl->lut[idx+i]=rgb[i];
        }
    }
    m_impl->dirty=false;
}

bool AtmosphericScattering::HasLUT()       const { return m_impl->lutW>0; }
uint32_t AtmosphericScattering::LUTWidth()  const { return m_impl->lutW; }
uint32_t AtmosphericScattering::LUTHeight() const { return m_impl->lutH; }
const float* AtmosphericScattering::LUTData() const { return m_impl->lut.empty()?nullptr:m_impl->lut.data(); }

void AtmosphericScattering::EvaluateSkyColour(const float viewDir[3], float outRGB[3]) const {
    if(!HasLUT()){ EvaluateSkyColourDirect(viewDir,outRGB); return; }
    float cosZ=viewDir[1];
    float cosS=m_impl->sunDir[1];
    float u=(1.f-cosS)*0.5f*(m_impl->lutW-1);
    float v=(1.f-cosZ)*0.5f*(m_impl->lutH-1);
    int xi=std::min((int)u,(int)m_impl->lutW-1), yi=std::min((int)v,(int)m_impl->lutH-1);
    uint32_t idx=(yi*m_impl->lutW+xi)*3;
    for(int i=0;i<3;i++) outRGB[i]=m_impl->lut[idx+i];
}
void AtmosphericScattering::EvaluateSkyColourDirect(const float viewDir[3], float outRGB[3]) const {
    m_impl->Integrate(viewDir, outRGB);
}
void AtmosphericScattering::GetSunColour(float out[3]) const {
    out[0]=1.f; out[1]=0.95f; out[2]=0.85f; // simplified warm white
    for(int i=0;i<3;i++) out[i]*=m_impl->params.sunIntensity*0.1f;
}
void AtmosphericScattering::GetHorizonColour(float out[3]) const {
    float hd[3]={1,0,0}; EvaluateSkyColourDirect(hd,out);
}
bool AtmosphericScattering::IsDirty()   const { return m_impl->dirty; }
void AtmosphericScattering::ClearDirty(){ m_impl->dirty=false; }

} // namespace Engine
