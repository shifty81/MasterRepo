#include "Engine/Render/ColorGrading/ColorGrading/ColorGrading.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

// HSV helpers
static void RGBtoHSV(float r,float g,float b,float& h,float& s,float& v){
    float mx=std::max({r,g,b}), mn=std::min({r,g,b}), d=mx-mn;
    v=mx; s=(mx>0)?d/mx:0;
    if(d<1e-6f){h=0;return;}
    if(mx==r) h=(g-b)/d+(g<b?6.f:0.f);
    else if(mx==g) h=(b-r)/d+2.f;
    else h=(r-g)/d+4.f;
    h/=6.f;
}
static void HSVtoRGB(float h,float s,float v,float& r,float& g,float& b){
    float i=std::floor(h*6.f), f=h*6.f-i;
    float p=v*(1-s),q=v*(1-f*s),t_=v*(1-(1-f)*s);
    int ii=(int)i%6;
    if(ii==0){r=v;g=t_;b=p;}else if(ii==1){r=q;g=v;b=p;}
    else if(ii==2){r=p;g=v;b=t_;}else if(ii==3){r=p;g=q;b=v;}
    else if(ii==4){r=t_;g=p;b=v;}else{r=v;g=p;b=q;}
}

struct ColorGrading::Impl {
    ColorGradingSettings settings;
    bool dirty{true};
    std::vector<float> lut;  // lutSize^3 * 3
    uint32_t lutSize{0};
};

ColorGrading::ColorGrading()  : m_impl(new Impl){}
ColorGrading::~ColorGrading() { Shutdown(); delete m_impl; }
void ColorGrading::Init()     {}
void ColorGrading::Shutdown() { m_impl->lut.clear(); }

void ColorGrading::SetSettings(const ColorGradingSettings& s){ m_impl->settings=s; m_impl->dirty=true; }
const ColorGradingSettings& ColorGrading::GetSettings() const { return m_impl->settings; }
void ColorGrading::LerpSettings(const ColorGradingSettings& a, const ColorGradingSettings& b, float t){
    ColorGradingSettings s;
    s.exposure   =a.exposure   +(b.exposure   -a.exposure   )*t;
    s.contrast   =a.contrast   +(b.contrast   -a.contrast   )*t;
    s.saturation =a.saturation +(b.saturation -a.saturation )*t;
    s.hueShift   =a.hueShift   +(b.hueShift   -a.hueShift   )*t;
    for(int i=0;i<3;i++){
        s.lgg.lift [i]=a.lgg.lift [i]+(b.lgg.lift [i]-a.lgg.lift [i])*t;
        s.lgg.gamma[i]=a.lgg.gamma[i]+(b.lgg.gamma[i]-a.lgg.gamma[i])*t;
        s.lgg.gain [i]=a.lgg.gain [i]+(b.lgg.gain [i]-a.lgg.gain [i])*t;
    }
    SetSettings(s);
}
bool ColorGrading::IsSettingsDirty() const { return m_impl->dirty; }
void ColorGrading::ClearDirty(){ m_impl->dirty=false; }

void ColorGrading::ApplyPreset(const std::string& name){
    ColorGradingSettings s;
    if(name=="warm"){s.hueShift=-5.f;s.saturation=1.1f;s.lgg.gain[0]=1.1f;s.lgg.gain[2]=0.9f;}
    else if(name=="cold"){s.hueShift=5.f;s.saturation=0.9f;s.lgg.gain[0]=0.9f;s.lgg.gain[2]=1.1f;}
    else if(name=="vintage"){s.contrast=0.85f;s.saturation=0.8f;s.exposure=-0.2f;}
    else if(name=="noir"){s.saturation=0.f;s.contrast=1.3f;}
    else if(name=="vivid"){s.saturation=1.5f;s.contrast=1.1f;}
    // neutral: default
    SetSettings(s);
}

bool ColorGrading::LoadLUT3D(const std::string& /*path*/, uint32_t lutSize){
    // Stub: generate identity LUT
    UploadLUT3D(nullptr, lutSize); return true;
}

void ColorGrading::UploadLUT3D(const float* data, uint32_t ls){
    m_impl->lutSize=ls;
    m_impl->lut.resize(ls*ls*ls*3);
    if(data){ std::copy(data,data+ls*ls*ls*3,m_impl->lut.begin()); }
    else { // identity
        for(uint32_t b=0;b<ls;b++) for(uint32_t g=0;g<ls;g++) for(uint32_t r=0;r<ls;r++){
            uint32_t idx=(b*ls*ls+g*ls+r)*3;
            m_impl->lut[idx+0]=(float)r/(ls-1);
            m_impl->lut[idx+1]=(float)g/(ls-1);
            m_impl->lut[idx+2]=(float)b/(ls-1);
        }
    }
    m_impl->dirty=true;
}
void ColorGrading::ClearLUT(){ m_impl->lut.clear(); m_impl->lutSize=0; }
bool ColorGrading::HasLUT()  const { return m_impl->lutSize>0; }
uint32_t ColorGrading::LUTSize() const { return m_impl->lutSize; }
const float* ColorGrading::LUTData() const { return m_impl->lut.empty()?nullptr:m_impl->lut.data(); }
float ColorGrading::GetLUTTexelSize() const { return m_impl->lutSize>0?1.f/m_impl->lutSize:0.f; }

void ColorGrading::Apply(uint8_t* buf, uint32_t w, uint32_t h) const
{
    const auto& s=m_impl->settings;
    float exposureMul=std::pow(2.f,s.exposure);
    for(uint32_t i=0;i<w*h;i++){
        float r=buf[i*4+0]/255.f, g=buf[i*4+1]/255.f, b_=buf[i*4+2]/255.f;
        // Exposure
        r*=exposureMul; g*=exposureMul; b_*=exposureMul;
        // Lift/Gamma/Gain
        for(int c=0;c<3;c++){
            float& ch=(c==0)?r:(c==1)?g:b_;
            float lft=s.lgg.lift[c], gam=s.lgg.gamma[c], gain=s.lgg.gain[c];
            ch=(ch+lft)*gain; if(ch>0) ch=std::pow(ch,1.f/gam);
        }
        // Contrast
        float mid=0.5f;
        r=(r-mid)*s.contrast+mid; g=(g-mid)*s.contrast+mid; b_=(b_-mid)*s.contrast+mid;
        // Saturation
        float lum=0.2126f*r+0.7152f*g+0.0722f*b_;
        r=lum+(r-lum)*s.saturation; g=lum+(g-lum)*s.saturation; b_=lum+(b_-lum)*s.saturation;
        // Hue shift
        if(std::abs(s.hueShift)>0.01f){
            float hh,ss,vv; RGBtoHSV(r,g,b_,hh,ss,vv);
            hh=std::fmod(hh+s.hueShift/360.f+1.f,1.f);
            HSVtoRGB(hh,ss,vv,r,g,b_);
        }
        buf[i*4+0]=(uint8_t)std::max(0.f,std::min(255.f,r*255.f));
        buf[i*4+1]=(uint8_t)std::max(0.f,std::min(255.f,g*255.f));
        buf[i*4+2]=(uint8_t)std::max(0.f,std::min(255.f,b_*255.f));
    }
}

} // namespace Engine
