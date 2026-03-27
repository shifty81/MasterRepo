#include "Engine/Render/ProceduralSky/ProceduralSky/ProceduralSky.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

static const float kPi=3.14159265f;

static inline float Dot3(SkyVec3 a, SkyVec3 b){ return a.x*b.x+a.y*b.y+a.z*b.z; }
static inline float Len3(SkyVec3 v){ return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); }
static inline SkyVec3 Norm3(SkyVec3 v){ float l=Len3(v); return l>0?SkyVec3{v.x/l,v.y/l,v.z/l}:SkyVec3{0,1,0}; }

struct StarEntry { SkyVec3 dir; float brightness; };

struct ProceduralSky::Impl {
    SkyVec3  sunDir    {0.577f,0.577f,0.577f};
    float    rayleigh  {0.0025f};
    float    mie       {0.001f};
    float    mieAniso  {0.76f};
    float    exposure  {1.f};
    float    timeOfDay {12.f};
    float    latitude  {45.f};
    bool     starsOn   {true};
    uint32_t starCount {800};
    float    starBright{0.8f};
    bool     moonOn    {false};
    float    moonPhase {0.f};
    SkyVec3  moonDir   {-0.577f,0.4f,0.577f};
    SkyRGB   fogColour {0.5f,0.6f,0.7f};
    float    fogDensity{0.02f};
    std::vector<StarEntry> stars;
    bool     starsDirty{true};

    void RebuildStars(){
        stars.clear(); stars.reserve(starCount);
        uint32_t seed=42;
        for(uint32_t i=0;i<starCount;i++){
            seed=seed*1664525+1013904223;
            float theta=((seed>>16)&0xFFFF)/65535.f*2*kPi;
            seed=seed*1664525+1013904223;
            float phi=((seed>>16)&0xFFFF)/65535.f*kPi;
            StarEntry e;
            e.dir={std::sin(phi)*std::cos(theta), std::cos(phi), std::sin(phi)*std::sin(theta)};
            seed=seed*1664525+1013904223;
            e.brightness=((seed>>16)&0xFFFF)/65535.f*starBright;
            stars.push_back(e);
        }
        starsDirty=false;
    }

    SkyRGB SunColourFromAtm() const {
        float cosTheta=std::max(0.f,sunDir.y); // elevation
        float rr=1.f-rayleigh*10.f*(1.f-cosTheta);
        float gg=1.f-rayleigh*8.f*(1.f-cosTheta);
        float bb=1.f; // blue stays
        return {std::max(0.f,rr),std::max(0.f,gg),bb};
    }
};

ProceduralSky::ProceduralSky(): m_impl(new Impl){}
ProceduralSky::~ProceduralSky(){ Shutdown(); delete m_impl; }
void ProceduralSky::Init(){}
void ProceduralSky::Shutdown(){}
void ProceduralSky::Reset(){ *m_impl=Impl{}; }

void ProceduralSky::SetSunDirection(SkyVec3 d){ m_impl->sunDir=Norm3(d); }
void ProceduralSky::SetTimeOfDay(float h){
    m_impl->timeOfDay=h;
    float angle=(h-6.f)/12.f*kPi; // sunrise=6, noon=12, sunset=18
    float latRad=m_impl->latitude*kPi/180.f;
    m_impl->sunDir={std::cos(angle)*std::cos(latRad), std::sin(angle), std::cos(angle)*std::sin(latRad)};
    m_impl->sunDir=Norm3(m_impl->sunDir);
}
void ProceduralSky::SetLatitude    (float d){ m_impl->latitude=d; }
void ProceduralSky::SetRayleighCoeff(float r){ m_impl->rayleigh=r; }
void ProceduralSky::SetMieCoeff    (float m){ m_impl->mie=m; }
void ProceduralSky::SetMieAnisotropy(float g){ m_impl->mieAniso=g; }
void ProceduralSky::SetExposure    (float e){ m_impl->exposure=e; }
void ProceduralSky::EnableStars    (bool on){ m_impl->starsOn=on; }
void ProceduralSky::SetStarDensity (uint32_t n){ m_impl->starCount=n; m_impl->starsDirty=true; }
void ProceduralSky::SetStarBrightness(float b){ m_impl->starBright=b; m_impl->starsDirty=true; }
void ProceduralSky::EnableMoon     (bool on){ m_impl->moonOn=on; }
void ProceduralSky::SetMoonPhase   (float p){ m_impl->moonPhase=p; }
void ProceduralSky::SetMoonDirection(SkyVec3 d){ m_impl->moonDir=Norm3(d); }
void ProceduralSky::SetHorizonFog  (SkyRGB col, float dens){ m_impl->fogColour=col; m_impl->fogDensity=dens; }

SkyRGBA ProceduralSky::Sample(SkyVec3 viewDir) const {
    viewDir=Norm3(viewDir);
    float cosTheta=std::max(0.f,Dot3(viewDir,m_impl->sunDir));
    float elevation=std::max(0.f,viewDir.y);

    // Rayleigh (sky)
    float ray=m_impl->rayleigh;
    float br=0.5f+0.5f*(1.f-ray*10.f)*elevation;
    float bg=0.6f+0.4f*elevation*(1.f-ray*5.f);
    float bb=0.9f+0.1f*elevation;

    // Mie (sun glow) using Henyey-Greenstein
    float g=m_impl->mieAniso;
    float phase=(1.f-g*g)/(4*kPi*std::pow(1.f+g*g-2*g*cosTheta,1.5f));
    float mie_k=m_impl->mie*phase;
    br+=mie_k*2; bg+=mie_k*1.5f; bb+=mie_k;

    // Sun disc
    if(cosTheta>0.9997f){ br=3; bg=3; bb=3; }

    // Horizon fog blend
    float fogBlend=std::max(0.f,1.f-elevation*10.f)*m_impl->fogDensity;
    fogBlend=std::min(1.f,fogBlend);
    br=br*(1-fogBlend)+m_impl->fogColour.r*fogBlend;
    bg=bg*(1-fogBlend)+m_impl->fogColour.g*fogBlend;
    bb=bb*(1-fogBlend)+m_impl->fogColour.b*fogBlend;

    // Stars (below horizon → none)
    if(m_impl->starsOn && elevation>0.05f && m_impl->sunDir.y<0.1f){
        if(m_impl->starsDirty) const_cast<Impl*>(m_impl)->RebuildStars();
        for(auto& s:m_impl->stars){
            float dot=Dot3(viewDir,s.dir);
            if(dot>0.9999f){ br+=s.brightness; bg+=s.brightness; bb+=s.brightness; break; }
        }
    }

    // Moon
    if(m_impl->moonOn && m_impl->moonDir.y>0){
        float moonDot=Dot3(viewDir,m_impl->moonDir);
        if(moonDot>0.9992f){
            float mp=(1.f-m_impl->moonPhase);
            br+=mp*0.8f; bg+=mp*0.8f; bb+=mp*0.9f;
        }
    }

    // Exposure
    float ev=m_impl->exposure;
    auto tone=[&](float v){ return 1.f-std::exp(-v*ev); };
    return {tone(br),tone(bg),tone(bb),1.f};
}

SkyRGB  ProceduralSky::GetSunColour   () const { return m_impl->SunColourFromAtm(); }
SkyRGB  ProceduralSky::GetAmbientColour() const {
    SkyRGB sc=m_impl->SunColourFromAtm();
    return {sc.r*0.3f, sc.g*0.35f, sc.b*0.4f};
}
SkyVec3 ProceduralSky::GetSunDirection() const { return m_impl->sunDir; }

void ProceduralSky::BakeCubemap(float* out, uint32_t sz) const {
    // 6 faces × sz × sz × 4 floats
    const SkyVec3 faces[6][3]={
        {{ 1,0,0},{0,-1,0},{0,0,-1}}, // +X
        {{-1,0,0},{0,-1,0},{0,0, 1}}, // -X
        {{ 0,1,0},{0, 0,1},{1,0, 0}}, // +Y
        {{ 0,-1,0},{0,0,-1},{1,0,0}}, // -Y
        {{ 0,0,1},{0,-1,0},{1,0, 0}}, // +Z
        {{ 0,0,-1},{0,-1,0},{-1,0,0}} // -Z
    };
    uint32_t facePixels=sz*sz*4;
    for(uint32_t f=0;f<6;f++){
        SkyVec3 right=faces[f][2], up=faces[f][1], fwd=faces[f][0];
        for(uint32_t y=0;y<sz;y++) for(uint32_t x=0;x<sz;x++){
            float u=((float)x+0.5f)/sz*2-1, v=((float)y+0.5f)/sz*2-1;
            SkyVec3 dir={fwd.x+right.x*u+up.x*v,
                         fwd.y+right.y*u+up.y*v,
                         fwd.z+right.z*u+up.z*v};
            SkyRGBA c=Sample(dir);
            uint32_t idx=f*facePixels+(y*sz+x)*4;
            out[idx+0]=c.r; out[idx+1]=c.g; out[idx+2]=c.b; out[idx+3]=c.a;
        }
    }
}

} // namespace Engine
