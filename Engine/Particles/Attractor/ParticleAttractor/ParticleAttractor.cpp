#include "Engine/Particles/Attractor/ParticleAttractor/ParticleAttractor.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

struct AttractorEntry {
    uint32_t       id;
    AttractorType  type;
    PAVec3         pos;
    AttractorParams params;
    bool           enabled{true};
};

static inline float Dot3(PAVec3 a, PAVec3 b){ return a.x*b.x+a.y*b.y+a.z*b.z; }
static inline float Len3sq(float dx,float dy,float dz){ return dx*dx+dy*dy+dz*dz; }
static inline float Len3(float dx,float dy,float dz){ return std::sqrt(dx*dx+dy*dy+dz*dz); }

struct ParticleAttractor::Impl {
    std::vector<AttractorEntry> attractors;
    uint32_t nextId{1};
    AttractorEntry* Find(uint32_t id){
        for(auto& a:attractors) if(a.id==id) return &a;
        return nullptr;
    }
};

ParticleAttractor::ParticleAttractor(): m_impl(new Impl){}
ParticleAttractor::~ParticleAttractor(){ Shutdown(); delete m_impl; }
void ParticleAttractor::Init(){}
void ParticleAttractor::Shutdown(){ m_impl->attractors.clear(); }
void ParticleAttractor::Reset(){ m_impl->attractors.clear(); m_impl->nextId=1; }

uint32_t ParticleAttractor::CreateAttractor(AttractorType type, PAVec3 pos, const AttractorParams& p){
    AttractorEntry e; e.id=m_impl->nextId++; e.type=type; e.pos=pos; e.params=p;
    m_impl->attractors.push_back(e); return e.id;
}
void ParticleAttractor::RemoveAttractor(uint32_t id){
    m_impl->attractors.erase(std::remove_if(m_impl->attractors.begin(),m_impl->attractors.end(),
        [id](const AttractorEntry& a){return a.id==id;}),m_impl->attractors.end());
}
void ParticleAttractor::SetAttractorPos    (uint32_t id, PAVec3 p){ auto* a=m_impl->Find(id); if(a) a->pos=p; }
void ParticleAttractor::SetAttractorEnabled(uint32_t id, bool en)  { auto* a=m_impl->Find(id); if(a) a->enabled=en; }
void ParticleAttractor::SetStrength        (uint32_t id, float s)  { auto* a=m_impl->Find(id); if(a) a->params.strength=s; }
void ParticleAttractor::SetRadius          (uint32_t id, float r)  { auto* a=m_impl->Find(id); if(a) a->params.radius=r; }
void ParticleAttractor::SetFalloff         (uint32_t id, float e)  { auto* a=m_impl->Find(id); if(a) a->params.falloff=e; }

void ParticleAttractor::ApplyToParticles(float* pos, float* vel,
                                          const float* masses, uint32_t count, float dt) const {
    for(auto& a:m_impl->attractors){
        if(!a.enabled) continue;
        const float R=a.params.radius; const float exp=a.params.falloff;
        const float str=a.params.strength;
        for(uint32_t i=0;i<count;i++){
            float px=pos[i*3+0], py=pos[i*3+1], pz=pos[i*3+2];
            float dx=a.pos.x-px, dy=a.pos.y-py, dz=a.pos.z-pz;
            float dist=Len3(dx,dy,dz);
            if(dist>R||dist<1e-6f) continue;
            float falloffFactor=1.f-std::pow(dist/R, exp);
            float force=str*falloffFactor;
            float mass=(masses&&masses[i]>0)?masses[i]:1.f;
            float accel=force/mass;

            switch(a.type){
                case AttractorType::Point:{
                    float invD=1.f/dist;
                    vel[i*3+0]+=dx*invD*accel*dt;
                    vel[i*3+1]+=dy*invD*accel*dt;
                    vel[i*3+2]+=dz*invD*accel*dt;
                    break;
                }
                case AttractorType::Ring:{
                    // Project onto ring plane, attract toward ring surface
                    PAVec3 ax=a.params.axis;
                    float proj=dx*ax.x+dy*ax.y+dz*ax.z;
                    float rx=dx-proj*ax.x, ry=dy-proj*ax.y, rz=dz-proj*ax.z;
                    float rLen=Len3(rx,ry,rz);
                    float toRing=rLen-a.params.ringRadius;
                    if(std::fabs(toRing)<1e-6f) break;
                    float dir=(toRing>0)?-1.f:1.f;
                    float invR=(rLen>1e-6f)?1.f/rLen:0;
                    vel[i*3+0]+=rx*invR*dir*accel*dt;
                    vel[i*3+1]+=ry*invR*dir*accel*dt;
                    vel[i*3+2]+=rz*invR*dir*accel*dt;
                    break;
                }
                case AttractorType::Vortex:{
                    PAVec3 ax=a.params.axis;
                    float invD=1.f/dist;
                    // Tangential component: cross(axis, dir-to-particle)
                    float ndx=dx*invD, ndy=dy*invD, ndz=dz*invD;
                    float cx=ax.y*ndz-ax.z*ndy;
                    float cy=ax.z*ndx-ax.x*ndz;
                    float cz=ax.x*ndy-ax.y*ndx;
                    vel[i*3+0]+=cx*accel*dt;
                    vel[i*3+1]+=cy*accel*dt;
                    vel[i*3+2]+=cz*accel*dt;
                    // Axial pull
                    vel[i*3+0]-=dx*invD*accel*0.2f*dt;
                    vel[i*3+1]-=dy*invD*accel*0.2f*dt;
                    vel[i*3+2]-=dz*invD*accel*0.2f*dt;
                    break;
                }
                case AttractorType::Plane:{
                    // Push/pull toward plane defined by pos+axis (one-sided)
                    PAVec3 ax=a.params.axis;
                    float side=dx*ax.x+dy*ax.y+dz*ax.z;
                    if(side>0){
                        vel[i*3+0]+=ax.x*accel*dt;
                        vel[i*3+1]+=ax.y*accel*dt;
                        vel[i*3+2]+=ax.z*accel*dt;
                    }
                    break;
                }
            }
        }
    }
}

uint32_t ParticleAttractor::GetAttractorCount() const { return (uint32_t)m_impl->attractors.size(); }

} // namespace Engine
