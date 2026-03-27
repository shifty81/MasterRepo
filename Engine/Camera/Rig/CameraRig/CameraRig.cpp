#include "Engine/Camera/Rig/CameraRig/CameraRig.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <vector>

namespace Engine {

static inline CRVec3 Add3(CRVec3 a,CRVec3 b){ return {a.x+b.x,a.y+b.y,a.z+b.z}; }
static inline CRVec3 Sub3(CRVec3 a,CRVec3 b){ return {a.x-b.x,a.y-b.y,a.z-b.z}; }
static inline CRVec3 Scale3(CRVec3 v,float s){ return {v.x*s,v.y*s,v.z*s}; }
static inline float  Dot3(CRVec3 a,CRVec3 b){ return a.x*b.x+a.y*b.y+a.z*b.z; }
static inline float  Len3(CRVec3 v){ return std::sqrt(Dot3(v,v)); }
static inline CRVec3 Norm3(CRVec3 v){ float l=Len3(v); return l>1e-6f?Scale3(v,1/l):CRVec3{0,0,0}; }

struct SecondaryTarget { uint32_t id; CRVec3 pos; float weight; };

struct CameraRig::Impl {
    CRVec3 targetPos    {0,0,0};
    CRVec3 targetVel    {0,0,0};
    CRVec3 offset       {0,3,-6};
    CRVec3 camPos       {0,3,-6};
    CRVec3 camVel       {0,0,0};
    CRVec3 lookAt       {0,0,0};
    float  stiffness    {9.f};
    float  damping      {6.f};
    float  lookAhead    {0.3f};
    float  avoidRadius  {0.5f};
    float  pitchMin     {-80.f};
    float  pitchMax     {80.f};
    float  distMin      {1.f};
    float  distMax      {20.f};
    float  fov          {60.f};

    std::vector<SecondaryTarget> secondaryTargets;
    std::function<float(CRVec3,CRVec3,float)> obstacleQuery;

    CRVec3 WeightedTargetPos() const {
        CRVec3 wp=targetPos; float totalW=1.f;
        for(auto& st:secondaryTargets){ wp=Add3(wp,Scale3(st.pos,st.weight)); totalW+=st.weight; }
        return Scale3(wp, 1.f/totalW);
    }
};

CameraRig::CameraRig(): m_impl(new Impl){}
CameraRig::~CameraRig(){ Shutdown(); delete m_impl; }
void CameraRig::Init(){}
void CameraRig::Shutdown(){}
void CameraRig::Reset(){ *m_impl = Impl{}; }

void CameraRig::SetTarget(CRVec3 pos, CRVec3 vel){ m_impl->targetPos=pos; m_impl->targetVel=vel; }
void CameraRig::AddSecondaryTarget(uint32_t id, CRVec3 pos, float w){
    for(auto& st:m_impl->secondaryTargets) if(st.id==id){ st.pos=pos; st.weight=w; return; }
    m_impl->secondaryTargets.push_back({id,pos,w});
}
void CameraRig::RemoveSecondaryTarget(uint32_t id){
    m_impl->secondaryTargets.erase(
        std::remove_if(m_impl->secondaryTargets.begin(),m_impl->secondaryTargets.end(),
            [id](const SecondaryTarget& s){return s.id==id;}),
        m_impl->secondaryTargets.end());
}
void CameraRig::UpdateSecondaryTarget(uint32_t id, CRVec3 pos){
    for(auto& st:m_impl->secondaryTargets) if(st.id==id){ st.pos=pos; return; }
}

void CameraRig::SetOffset         (CRVec3 o){ m_impl->offset=o; }
void CameraRig::SetSpring         (float k,float d){ m_impl->stiffness=k; m_impl->damping=d; }
void CameraRig::SetLookAheadFactor(float k){ m_impl->lookAhead=k; }
void CameraRig::SetAvoidRadius    (float r){ m_impl->avoidRadius=r; }
void CameraRig::SetPitchLimits    (float mn,float mx){ m_impl->pitchMin=mn; m_impl->pitchMax=mx; }
void CameraRig::SetDistanceLimits (float mn,float mx){ m_impl->distMin=mn; m_impl->distMax=mx; }
void CameraRig::SetFOV            (float d){ m_impl->fov=d; }
void CameraRig::SetOnObstacleQuery(std::function<float(CRVec3,CRVec3,float)> fn){ m_impl->obstacleQuery=fn; }

CameraRigState CameraRig::Update(float dt){
    // Compute look-ahead target
    CRVec3 anchor=m_impl->WeightedTargetPos();
    CRVec3 lookahead=Add3(anchor,Scale3(m_impl->targetVel,m_impl->lookAhead));
    CRVec3 desired=Add3(lookahead,m_impl->offset);

    // Spring update
    CRVec3 disp=Sub3(desired,m_impl->camPos);
    CRVec3 accel=Sub3(Scale3(disp,m_impl->stiffness),Scale3(m_impl->camVel,m_impl->damping));
    m_impl->camVel=Add3(m_impl->camVel,Scale3(accel,dt));
    m_impl->camPos=Add3(m_impl->camPos,Scale3(m_impl->camVel,dt));

    // Distance clamping
    CRVec3 toTarget=Sub3(anchor,m_impl->camPos);
    float d=Len3(toTarget);
    if(d<m_impl->distMin&&d>1e-6f)
        m_impl->camPos=Sub3(anchor,Scale3(Norm3(toTarget),m_impl->distMin));
    else if(d>m_impl->distMax)
        m_impl->camPos=Sub3(anchor,Scale3(Norm3(toTarget),m_impl->distMax));

    // Simple pitch clamp (project camera vector to xz plane)
    CRVec3 dir=Sub3(anchor,m_impl->camPos);
    float hz=std::sqrt(dir.x*dir.x+dir.z*dir.z);
    float pitch=(hz>1e-6f)?std::atan2(dir.y,hz)*57.2958f:0;
    if(pitch<m_impl->pitchMin||pitch>m_impl->pitchMax){
        float clampedPitch=std::max(m_impl->pitchMin,std::min(m_impl->pitchMax,pitch));
        float cpRad=clampedPitch/57.2958f;
        float len=Len3(dir);
        float yaw=std::atan2(dir.x,dir.z);
        dir={std::sin(yaw)*std::cos(cpRad)*len,
             std::sin(cpRad)*len,
             std::cos(yaw)*std::cos(cpRad)*len};
        m_impl->camPos=Sub3(anchor,dir);
    }

    // Obstacle avoidance (contract if blocked)
    if(m_impl->obstacleQuery){
        CRVec3 rayDir=Norm3(Sub3(anchor,m_impl->camPos));
        float dist=Len3(Sub3(anchor,m_impl->camPos));
        float hit=m_impl->obstacleQuery(anchor,Scale3(rayDir,-1),dist);
        if(hit<dist)
            m_impl->camPos=Add3(anchor,Scale3(Scale3(rayDir,-1),hit-m_impl->avoidRadius));
    }

    m_impl->lookAt=anchor;
    return {m_impl->camPos, m_impl->lookAt, m_impl->fov};
}

CRVec3 CameraRig::GetPosition() const { return m_impl->camPos; }
CRVec3 CameraRig::GetLookAt  () const { return m_impl->lookAt; }
float  CameraRig::GetFOV     () const { return m_impl->fov; }

} // namespace Engine
