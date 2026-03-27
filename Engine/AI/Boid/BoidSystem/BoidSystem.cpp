#include "Engine/AI/Boid/BoidSystem/BoidSystem.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace Engine {

static float Len3(const float v[3]){ return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])+1e-9f; }

struct Boid { float pos[3]{}, vel[3]{}; };

struct FlockObstacle { float centre[3]{}; float radius; };

struct FlockData {
    uint32_t          id;
    BoidFlockDesc     desc;
    std::vector<Boid> boids;
    float             goal[3]{};
    float             goalWeight{0.f};
    bool              hasGoal{false};
    float             boundary[3]{};
    float             boundaryHalfExtent;
    std::vector<FlockObstacle> obstacles;
};

struct BoidSystem::Impl {
    std::vector<FlockData*> flocks;
    uint32_t nextId{1};

    FlockData* Find(uint32_t id){ for(auto* f:flocks) if(f->id==id) return f; return nullptr; }
    const FlockData* Find(uint32_t id) const { for(auto* f:flocks) if(f->id==id) return f; return nullptr; }

    void ComputeBoid(FlockData& fd, uint32_t i, float dt){
        Boid& b=fd.boids[i];
        float coh[3]={},sep[3]={},ali[3]={};
        uint32_t cohN=0, aliN=0;
        for(uint32_t j=0;j<(uint32_t)fd.boids.size();j++){
            if(i==j) continue;
            Boid& nb=fd.boids[j];
            float d[3]={nb.pos[0]-b.pos[0],nb.pos[1]-b.pos[1],nb.pos[2]-b.pos[2]};
            float dist=Len3(d);
            if(dist<fd.desc.neighbourRadius){
                for(int k=0;k<3;k++) coh[k]+=nb.pos[k];
                for(int k=0;k<3;k++) ali[k]+=nb.vel[k];
                cohN++; aliN++;
            }
            if(dist<fd.desc.separationDist && dist>0.f){
                for(int k=0;k<3;k++) sep[k]-=d[k]/dist;
            }
        }
        float force[3]={};
        if(cohN>0){ for(int k=0;k<3;k++){coh[k]=coh[k]/cohN-b.pos[k]; float l=Len3(coh); coh[k]=coh[k]/l*fd.desc.maxSpeed-b.vel[k];} }
        if(aliN>0){ for(int k=0;k<3;k++){ali[k]/=aliN; float l=Len3(ali); ali[k]=ali[k]/l*fd.desc.maxSpeed-b.vel[k];} }
        float sl=Len3(sep); if(sl>1e-6f) for(int k=0;k<3;k++) sep[k]=sep[k]/sl*fd.desc.maxSpeed-b.vel[k];
        for(int k=0;k<3;k++) force[k]+=coh[k]*fd.desc.cohesionWeight+sep[k]*fd.desc.separationWeight+ali[k]*fd.desc.alignmentWeight;
        // Goal
        if(fd.hasGoal){ for(int k=0;k<3;k++){float g=fd.goal[k]-b.pos[k]; float gl=Len3(&fd.goal[0])+1e-9f; force[k]+=g/(gl)*fd.desc.maxSpeed*fd.goalWeight;} }
        // Boundary
        for(int k=0;k<3;k++){
            float lo=fd.boundary[k]-fd.boundaryHalfExtent, hi=fd.boundary[k]+fd.boundaryHalfExtent;
            if(b.pos[k]<lo+2.f) force[k]+=fd.desc.boundaryRepulsion;
            if(b.pos[k]>hi-2.f) force[k]-=fd.desc.boundaryRepulsion;
        }
        // Clamp force
        float fl=Len3(force); if(fl>fd.desc.maxForce){ float s=fd.desc.maxForce/fl; for(int k=0;k<3;k++) force[k]*=s; }
        for(int k=0;k<3;k++) b.vel[k]+=force[k]/fd.desc.mass*dt;
        float spd=Len3(b.vel);
        if(spd>fd.desc.maxSpeed){ float s=fd.desc.maxSpeed/spd; for(int k=0;k<3;k++) b.vel[k]*=s; }
        for(int k=0;k<3;k++) b.vel[k]*=(1.f-fd.desc.damping);
        for(int k=0;k<3;k++) b.pos[k]+=b.vel[k]*dt;
    }
};

BoidSystem::BoidSystem()  : m_impl(new Impl){}
BoidSystem::~BoidSystem() { Shutdown(); delete m_impl; }
void BoidSystem::Init()     {}
void BoidSystem::Shutdown() { for(auto* f:m_impl->flocks) delete f; m_impl->flocks.clear(); }

uint32_t BoidSystem::CreateFlock(const BoidFlockDesc& desc, const float origin[3], float spread)
{
    FlockData* fd=new FlockData; fd->id=m_impl->nextId++; fd->desc=desc;
    float ox=origin?origin[0]:0.f, oy=origin?origin[1]:0.f, oz=origin?origin[2]:0.f;
    fd->boundaryHalfExtent=desc.boundaryHalfExtent;
    for(int i=0;i<3;i++) fd->boundary[i]=origin?origin[i]:0.f;
    for(uint32_t i=0;i<desc.count;i++){
        Boid b;
        b.pos[0]=ox+(float(rand())/RAND_MAX*2.f-1.f)*spread;
        b.pos[1]=oy+(float(rand())/RAND_MAX*2.f-1.f)*spread;
        b.pos[2]=oz+(float(rand())/RAND_MAX*2.f-1.f)*spread;
        b.vel[0]=(float(rand())/RAND_MAX*2.f-1.f);
        b.vel[1]=(float(rand())/RAND_MAX*2.f-1.f);
        b.vel[2]=(float(rand())/RAND_MAX*2.f-1.f);
        fd->boids.push_back(b);
    }
    m_impl->flocks.push_back(fd); return fd->id;
}

void BoidSystem::DestroyFlock(uint32_t id){
    auto& v=m_impl->flocks;
    for(auto it=v.begin();it!=v.end();++it) if((*it)->id==id){delete *it; v.erase(it); return;}
}
bool BoidSystem::HasFlock(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void BoidSystem::SetGoal(uint32_t id, const float wp[3], float w){
    auto* f=m_impl->Find(id); if(f){for(int i=0;i<3;i++) f->goal[i]=wp[i]; f->goalWeight=w; f->hasGoal=true;}
}
void BoidSystem::ClearGoal(uint32_t id){ auto* f=m_impl->Find(id); if(f) f->hasGoal=false; }

void BoidSystem::AddObstacle(uint32_t id, const float c[3], float r){
    auto* f=m_impl->Find(id); if(f){ FlockObstacle o; for(int i=0;i<3;i++) o.centre[i]=c[i]; o.radius=r; f->obstacles.push_back(o); }
}
void BoidSystem::ClearObstacles(uint32_t id){ auto* f=m_impl->Find(id); if(f) f->obstacles.clear(); }

void BoidSystem::SetBoundary(uint32_t id, const float c[3], float he){
    auto* f=m_impl->Find(id); if(f){ for(int i=0;i<3;i++) f->boundary[i]=c[i]; f->boundaryHalfExtent=he; }
}
void BoidSystem::SetWeights(uint32_t id, float coh, float sep, float ali){
    auto* f=m_impl->Find(id); if(f){f->desc.cohesionWeight=coh;f->desc.separationWeight=sep;f->desc.alignmentWeight=ali;}
}

uint32_t BoidSystem::BoidCount(uint32_t id) const {
    const auto* f=m_impl->Find(id); return f?(uint32_t)f->boids.size():0;
}
const float* BoidSystem::GetBoidPos(uint32_t id, uint32_t bi) const {
    const auto* f=m_impl->Find(id); return (f&&bi<f->boids.size())?f->boids[bi].pos:nullptr;
}
const float* BoidSystem::GetBoidVel(uint32_t id, uint32_t bi) const {
    const auto* f=m_impl->Find(id); return (f&&bi<f->boids.size())?f->boids[bi].vel:nullptr;
}

std::vector<uint32_t> BoidSystem::GetAllFlocks() const {
    std::vector<uint32_t> out; for(auto* f:m_impl->flocks) out.push_back(f->id); return out;
}

void BoidSystem::SpawnBoid(uint32_t id, const float p[3], const float v[3]){
    auto* f=m_impl->Find(id); if(!f) return;
    Boid b; for(int i=0;i<3;i++){b.pos[i]=p[i]; b.vel[i]=v?v[i]:0.f;} f->boids.push_back(b);
}
void BoidSystem::DespawnBoid(uint32_t id, uint32_t bi){
    auto* f=m_impl->Find(id); if(f&&bi<f->boids.size()) f->boids.erase(f->boids.begin()+bi);
}

void BoidSystem::Tick(float dt){
    for(auto* fd:m_impl->flocks)
        for(uint32_t i=0;i<(uint32_t)fd->boids.size();i++)
            m_impl->ComputeBoid(*fd,i,dt);
}

} // namespace Engine
