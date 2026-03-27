#include "Engine/Particles/EmitterPool/ParticleEmitterPool/ParticleEmitterPool.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct EmitterInstance {
    uint32_t instanceId;
    uint32_t defId;
    float pos[3]{};
    bool  active{true};
    float spawnAccum{0};
    uint32_t particleCount{0};
    bool  inPool{false};
};

struct PoolEntry {
    std::unordered_map<uint32_t,EmitterDef>               defs;
    std::vector<EmitterInstance>                          pool;
    std::unordered_map<uint32_t,std::function<void(uint32_t)>> onSpawn, onReturn;
};

struct ParticleEmitterPool::Impl {
    PoolEntry       entry;
    uint32_t        nextInstId{1};
    uint32_t        maxActive{256};
    float           lodNear{20.f}, lodFar{100.f};
    float           listener[3]{};

    EmitterInstance* FindInst(uint32_t id){
        for(auto& e:entry.pool) if(e.instanceId==id&&!e.inPool) return &e;
        return nullptr;
    }
    float DistToListener(const EmitterInstance& e) const {
        float dx=e.pos[0]-listener[0],dy=e.pos[1]-listener[1],dz=e.pos[2]-listener[2];
        return std::sqrt(dx*dx+dy*dy+dz*dz);
    }
};

ParticleEmitterPool::ParticleEmitterPool(): m_impl(new Impl){}
ParticleEmitterPool::~ParticleEmitterPool(){ Shutdown(); delete m_impl; }
void ParticleEmitterPool::Init(){}
void ParticleEmitterPool::Shutdown(){ m_impl->entry.pool.clear(); m_impl->entry.defs.clear(); }
void ParticleEmitterPool::Reset(){ m_impl->entry.pool.clear(); }

bool ParticleEmitterPool::RegisterEmitter(const EmitterDef& def){
    if(m_impl->entry.defs.count(def.id)) return false;
    m_impl->entry.defs[def.id]=def;
    // Pre-fill pool
    for(uint32_t i=0;i<def.poolSize;i++){
        EmitterInstance ei; ei.instanceId=m_impl->nextInstId++;
        ei.defId=def.id; ei.inPool=true;
        m_impl->entry.pool.push_back(ei);
    }
    return true;
}

uint32_t ParticleEmitterPool::SpawnEmitter(uint32_t defId, float x, float y, float z){
    // Count active
    uint32_t active=0;
    for(auto& e:m_impl->entry.pool) if(!e.inPool) active++;
    if(active>=m_impl->maxActive) return 0;
    // Find pooled instance
    for(auto& e:m_impl->entry.pool){
        if(e.inPool&&e.defId==defId){
            e.inPool=false; e.active=true;
            e.pos[0]=x; e.pos[1]=y; e.pos[2]=z;
            e.spawnAccum=0; e.particleCount=0;
            auto it=m_impl->entry.onSpawn.find(defId);
            if(it!=m_impl->entry.onSpawn.end()) it->second(e.instanceId);
            return e.instanceId;
        }
    }
    return 0;
}
void ParticleEmitterPool::ReturnEmitter(uint32_t instanceId){
    auto* e=m_impl->FindInst(instanceId); if(!e) return;
    e->inPool=true; e->active=false;
    auto it=m_impl->entry.onReturn.find(e->defId);
    if(it!=m_impl->entry.onReturn.end()) it->second(instanceId);
}

void ParticleEmitterPool::SetEmitterPosition(uint32_t id, float x, float y, float z){
    auto* e=m_impl->FindInst(id); if(!e) return;
    e->pos[0]=x; e->pos[1]=y; e->pos[2]=z;
}
void ParticleEmitterPool::SetEmitterActive(uint32_t id, bool on){
    auto* e=m_impl->FindInst(id); if(e) e->active=on;
}
void ParticleEmitterPool::Burst(uint32_t id, uint32_t count){
    auto* e=m_impl->FindInst(id); if(!e) return;
    auto it=m_impl->entry.defs.find(e->defId);
    if(it!=m_impl->entry.defs.end())
        e->particleCount=std::min(e->particleCount+count,it->second.maxParticles);
}

void ParticleEmitterPool::Tick(float dt){
    for(auto& e:m_impl->entry.pool){
        if(e.inPool||!e.active) continue;
        float dist=m_impl->DistToListener(e);
        if(dist>m_impl->lodFar){ e.active=false; continue; }
        auto it=m_impl->entry.defs.find(e.defId);
        if(it==m_impl->entry.defs.end()) continue;
        auto& def=it->second;
        float lodMult=(dist>m_impl->lodNear)?1.f-(dist-m_impl->lodNear)/(m_impl->lodFar-m_impl->lodNear):1.f;
        e.spawnAccum+=def.spawnRate*lodMult*dt;
        while(e.spawnAccum>=1.f&&e.particleCount<def.maxParticles){
            e.particleCount++; e.spawnAccum-=1.f;
        }
        // Age particles simply
        if(e.particleCount>0) e.particleCount--;
    }
}

void ParticleEmitterPool::SetMaxActiveEmitters(uint32_t n){ m_impl->maxActive=std::max(1u,n); }
void ParticleEmitterPool::SetLODDistances(float n, float f){ m_impl->lodNear=n; m_impl->lodFar=f; }
void ParticleEmitterPool::SetListenerPos(float x, float y, float z){ m_impl->listener[0]=x;m_impl->listener[1]=y;m_impl->listener[2]=z; }

uint32_t ParticleEmitterPool::GetActiveCount() const {
    uint32_t n=0; for(auto& e:m_impl->entry.pool) if(!e.inPool) n++; return n;
}
uint32_t ParticleEmitterPool::GetPoolSize(uint32_t defId) const {
    uint32_t n=0; for(auto& e:m_impl->entry.pool) if(e.defId==defId) n++; return n;
}
void ParticleEmitterPool::SetOnSpawn (uint32_t defId,std::function<void(uint32_t)> cb){ m_impl->entry.onSpawn[defId]=cb; }
void ParticleEmitterPool::SetOnReturn(uint32_t defId,std::function<void(uint32_t)> cb){ m_impl->entry.onReturn[defId]=cb; }

} // namespace Engine
