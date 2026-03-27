#include "Engine/Input/AimAssist/AimAssist/AimAssist.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace Engine {

static inline float Dot3(AimVec3 a, AimVec3 b){ return a.x*b.x+a.y*b.y+a.z*b.z; }
static inline AimVec3 Sub3(AimVec3 a, AimVec3 b){ return {a.x-b.x,a.y-b.y,a.z-b.z}; }
static inline float Len3(AimVec3 v){ return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); }
static inline AimVec3 Norm3(AimVec3 v){ float l=Len3(v); if(l<1e-6f)l=1.f; return {v.x/l,v.y/l,v.z/l}; }
static inline AimVec3 Lerp3(AimVec3 a, AimVec3 b, float t){ return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t}; }

struct AimTarget { AimVec3 pos; float radius; };

struct AimAssist::Impl {
    std::unordered_map<uint32_t,AimTarget> targets;
    float magnetism{0.4f};
    float stickyR  {0.05f};  // radians
    float stickyS  {0.5f};
    float assistFoV{0.26f};  // ~15 degrees
    float maxDist  {50.f};
    mutable uint32_t lockedId{0};

    uint32_t FindBest(AimVec3 dir, AimVec3 camPos) const {
        uint32_t bestId=0; float bestScore=-1.f;
        for(auto& [id,t]:targets){
            AimVec3 toT=Sub3(t.pos,camPos);
            float dist=Len3(toT); if(dist>maxDist) continue;
            AimVec3 toTN=Norm3(toT);
            float cosA=Dot3(dir,toTN);
            float halfFoV=std::cos(assistFoV); if(cosA<halfFoV) continue;
            float score=cosA/(dist+1.f);
            if(score>bestScore){ bestScore=score; bestId=id; }
        }
        return bestId;
    }
};

AimAssist::AimAssist(): m_impl(new Impl){}
AimAssist::~AimAssist(){ Shutdown(); delete m_impl; }
void AimAssist::Init(){}
void AimAssist::Shutdown(){ m_impl->targets.clear(); }
void AimAssist::Reset(){ m_impl->targets.clear(); m_impl->lockedId=0; }

void AimAssist::RegisterTarget  (uint32_t id, AimVec3 p, float r){ m_impl->targets[id]={p,r}; }
void AimAssist::UnregisterTarget(uint32_t id){ m_impl->targets.erase(id); }
void AimAssist::UpdateTargetPos (uint32_t id, AimVec3 p){
    auto it=m_impl->targets.find(id); if(it!=m_impl->targets.end()) it->second.pos=p;
}

AimVec3 AimAssist::ComputeAssist(AimVec3 dir, AimVec3 camPos) const {
    dir=Norm3(dir);
    uint32_t best=m_impl->FindBest(dir,camPos);
    m_impl->lockedId=best;
    if(!best) return dir;
    AimVec3 toT=Norm3(Sub3(m_impl->targets[best].pos, camPos));
    return Norm3(Lerp3(dir, toT, m_impl->magnetism));
}

float AimAssist::StickyFactor(AimVec3 dir, AimVec3 camPos) const {
    dir=Norm3(dir);
    uint32_t best=m_impl->FindBest(dir,camPos);
    if(!best) return 1.f;
    AimVec3 toT=Norm3(Sub3(m_impl->targets[best].pos, camPos));
    float cosA=Dot3(dir,toT);
    float angle=std::acos(std::max(-1.f,std::min(1.f,cosA)));
    if(angle>m_impl->stickyR) return 1.f;
    float t=1.f-angle/m_impl->stickyR;
    return 1.f - t*m_impl->stickyS;
}

void AimAssist::SetMagnetismStrength(float s){ m_impl->magnetism=s; }
void AimAssist::SetStickyRadius     (float r){ m_impl->stickyR=r; }
void AimAssist::SetStickyStrength   (float s){ m_impl->stickyS=s; }
void AimAssist::SetAssistFoV        (float r){ m_impl->assistFoV=r; }
void AimAssist::SetMaxDistance      (float d){ m_impl->maxDist=d; }
uint32_t AimAssist::GetLockedTarget() const { return m_impl->lockedId; }

} // namespace Engine
