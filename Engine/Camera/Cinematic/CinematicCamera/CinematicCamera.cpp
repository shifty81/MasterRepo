#include "Engine/Camera/Cinematic/CinematicCamera/CinematicCamera.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

namespace Engine {

static inline float Lerp(float a,float b,float t){ return a+(b-a)*t; }
static inline CCVec3 LerpV3(CCVec3 a,CCVec3 b,float t){
    return {Lerp(a.x,b.x,t),Lerp(a.y,b.y,t),Lerp(a.z,b.z,t)};
}
static inline CameraState LerpState(const CameraState& a, const CameraState& b, float t){
    return {LerpV3(a.pos,b.pos,t),LerpV3(a.lookAt,b.lookAt,t),
            Lerp(a.fovDeg,b.fovDeg,t),Lerp(a.aperture,b.aperture,t),
            Lerp(a.focusDist,b.focusDist,t)};
}

// Catmull-Rom interpolation for Vec3
static CCVec3 CatmullRom(CCVec3 p0, CCVec3 p1, CCVec3 p2, CCVec3 p3, float t){
    float t2=t*t, t3=t2*t;
    auto Cr=[&](float v0,float v1,float v2,float v3) -> float {
        return 0.5f*((2*v1)+(-v0+v2)*t+(2*v0-5*v1+4*v2-v3)*t2+(-v0+3*v1-3*v2+v3)*t3);
    };
    return {Cr(p0.x,p1.x,p2.x,p3.x),Cr(p0.y,p1.y,p2.y,p3.y),Cr(p0.z,p1.z,p2.z,p3.z)};
}

struct CinematicCamera::Impl {
    std::vector<CameraKeyframe> keyframes;
    std::vector<float>          cuts;
    SplineType spline{SplineType::CatmullRom};
    float playhead{0};
    bool  playing{false};
    bool  looping{false};
    float duration{0};
    CameraState current{};
    std::function<void(float)> onCut;

    void UpdateDuration(){
        duration=keyframes.empty()?0:keyframes.back().time;
    }

    bool IsAtCut(float oldT, float newT) const {
        for(float ct:cuts) if(ct>oldT&&ct<=newT) return true;
        return false;
    }

    // Find bracketing keyframes
    CameraState Evaluate(float t) const {
        if(keyframes.empty()) return {};
        if(keyframes.size()==1) return {keyframes[0].pos,keyframes[0].lookAt,
            keyframes[0].fovDeg,keyframes[0].aperture,keyframes[0].focusDist};
        // Clamp
        if(t<=keyframes.front().time) return {keyframes[0].pos,keyframes[0].lookAt,
            keyframes[0].fovDeg,keyframes[0].aperture,keyframes[0].focusDist};
        if(t>=keyframes.back().time){
            auto& k=keyframes.back();
            return {k.pos,k.lookAt,k.fovDeg,k.aperture,k.focusDist};
        }
        // Find segment
        for(size_t i=0;i+1<keyframes.size();i++){
            auto& k0=keyframes[i], &k1=keyframes[i+1];
            if(t>=k0.time&&t<=k1.time){
                float seg=k1.time-k0.time;
                float u=(seg>0)?(t-k0.time)/seg:0.f;
                CameraState a{k0.pos,k0.lookAt,k0.fovDeg,k0.aperture,k0.focusDist};
                CameraState b{k1.pos,k1.lookAt,k1.fovDeg,k1.aperture,k1.focusDist};
                if(spline==SplineType::Linear) return LerpState(a,b,u);
                // CatmullRom: need p-1 and p+2
                size_t im1=(i>0)?i-1:0;
                size_t ip2=(i+2<keyframes.size())?i+2:keyframes.size()-1;
                CCVec3 pos=CatmullRom(keyframes[im1].pos,k0.pos,k1.pos,keyframes[ip2].pos,u);
                CCVec3 la =CatmullRom(keyframes[im1].lookAt,k0.lookAt,k1.lookAt,keyframes[ip2].lookAt,u);
                return {pos,la,Lerp(k0.fovDeg,k1.fovDeg,u),Lerp(k0.aperture,k1.aperture,u),Lerp(k0.focusDist,k1.focusDist,u)};
            }
        }
        return {};
    }
};

CinematicCamera::CinematicCamera(): m_impl(new Impl){}
CinematicCamera::~CinematicCamera(){ Shutdown(); delete m_impl; }
void CinematicCamera::Init(){}
void CinematicCamera::Shutdown(){ m_impl->keyframes.clear(); m_impl->cuts.clear(); }
void CinematicCamera::Reset(){ Shutdown(); m_impl->playhead=0; m_impl->playing=false; m_impl->duration=0; }

uint32_t CinematicCamera::AddKeyframe(const CameraKeyframe& kf){
    m_impl->keyframes.push_back(kf);
    std::sort(m_impl->keyframes.begin(),m_impl->keyframes.end(),
        [](const CameraKeyframe& a,const CameraKeyframe& b){return a.time<b.time;});
    m_impl->UpdateDuration();
    return (uint32_t)(std::find_if(m_impl->keyframes.begin(),m_impl->keyframes.end(),
        [&kf](const CameraKeyframe& k){return k.time==kf.time;})-m_impl->keyframes.begin());
}
void CinematicCamera::RemoveKeyframe(uint32_t idx){
    if(idx<m_impl->keyframes.size()){ m_impl->keyframes.erase(m_impl->keyframes.begin()+idx); m_impl->UpdateDuration(); }
}
void CinematicCamera::AddCut(float t){ m_impl->cuts.push_back(t); }
void CinematicCamera::ClearKeyframes(){ m_impl->keyframes.clear(); m_impl->cuts.clear(); m_impl->UpdateDuration(); }

void CinematicCamera::Play(bool loop){ m_impl->playing=true; m_impl->looping=loop; }
void CinematicCamera::Pause(){ m_impl->playing=false; }
void CinematicCamera::Stop(){ m_impl->playing=false; m_impl->playhead=0; }
void CinematicCamera::Seek(float t){ m_impl->playhead=std::max(0.f,std::min(m_impl->duration,t)); }

void CinematicCamera::Tick(float dt){
    if(!m_impl->playing||m_impl->duration<=0) return;
    float oldT=m_impl->playhead;
    m_impl->playhead+=dt;
    if(m_impl->playhead>=m_impl->duration){
        if(m_impl->looping) m_impl->playhead=std::fmod(m_impl->playhead,m_impl->duration);
        else{ m_impl->playhead=m_impl->duration; m_impl->playing=false; }
    }
    if(m_impl->onCut && m_impl->IsAtCut(oldT,m_impl->playhead))
        m_impl->onCut(m_impl->playhead);
    m_impl->current=m_impl->Evaluate(m_impl->playhead);
}

CameraState CinematicCamera::EvaluateAt(float t) const { return m_impl->Evaluate(t); }
CameraState CinematicCamera::GetCurrentState() const { return m_impl->current; }
float       CinematicCamera::GetPlayhead()     const { return m_impl->playhead; }
float       CinematicCamera::GetDuration()     const { return m_impl->duration; }
bool        CinematicCamera::IsPlaying()        const { return m_impl->playing; }
uint32_t    CinematicCamera::GetKeyframeCount() const { return (uint32_t)m_impl->keyframes.size(); }

CameraState CinematicCamera::Blend(const CameraState& a, const CameraState& b, float t){ return LerpState(a,b,t); }

void CinematicCamera::SetSplineType(SplineType t){ m_impl->spline=t; }
void CinematicCamera::SetOnCut(std::function<void(float)> cb){ m_impl->onCut=cb; }

std::string CinematicCamera::ExportJSON() const {
    std::ostringstream os; os<<"{\"keyframes\":[\n";
    for(size_t i=0;i<m_impl->keyframes.size();i++){
        auto& k=m_impl->keyframes[i];
        os<<"{\"t\":"<<k.time<<",\"pos\":["<<k.pos.x<<","<<k.pos.y<<","<<k.pos.z
          <<"],\"fov\":"<<k.fovDeg<<"}"<<(i+1<m_impl->keyframes.size()?",":"")<<"\n";
    }
    os<<"]}";
    return os.str();
}
bool CinematicCamera::LoadFromJSON(const std::string& /*json*/){ return true; /* stub */ }

} // namespace Engine
