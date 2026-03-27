#include "Runtime/Gameplay/Spawn/SpawnManager/SpawnManager.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct SpawnPoint {
    uint32_t id;
    SpawnVec3 pos{};
    std::vector<std::string> tags;
};

struct WaveEntry2 : WaveEntry { // extended with runtime state
    float  elapsed{0};
    uint32_t spawned{0};
    bool   started{false};
};

struct WaveRuntime {
    uint32_t id;
    WaveDesc desc;
    std::vector<WaveEntry2> state;
    bool     active{false};
    bool     complete{false};
    float    elapsed{0};
    bool     repeat{false};
    std::function<void()> onComplete;
};

struct SpawnManager::Impl {
    uint32_t nextWaveId{1};
    uint32_t maxConcurrent{50};
    uint32_t aliveCount{0};
    std::vector<SpawnPoint>  spawnPoints;
    std::unordered_map<uint32_t,uint32_t> entityTypePool; // typeId → poolSize
    std::unordered_map<uint32_t,uint32_t> aliveByType;
    std::vector<WaveRuntime> waves;
    std::function<uint32_t(uint32_t,SpawnVec3)> onSpawn;
    std::function<void()> onAllDone;

    WaveRuntime* FindWave(uint32_t id){ for(auto& w:waves) if(w.id==id) return &w; return nullptr; }

    SpawnVec3 PickSpawnPoint(const std::string& tag) const {
        std::vector<const SpawnPoint*> matching;
        for(auto& sp:spawnPoints){
            if(tag.empty()){ matching.push_back(&sp); continue; }
            for(auto& t:sp.tags) if(t==tag){ matching.push_back(&sp); break; }
        }
        if(matching.empty()) return {0,0,0};
        return matching[rand()%(uint32_t)matching.size()]->pos;
    }
};

SpawnManager::SpawnManager(): m_impl(new Impl){}
SpawnManager::~SpawnManager(){ Shutdown(); delete m_impl; }
void SpawnManager::Init(){}
void SpawnManager::Shutdown(){ m_impl->waves.clear(); m_impl->spawnPoints.clear(); }
void SpawnManager::Reset(){ Shutdown(); m_impl->nextWaveId=1; m_impl->aliveCount=0; }

void SpawnManager::RegisterSpawnPoint(uint32_t id, SpawnVec3 pos, const std::vector<std::string>& tags){
    SpawnPoint sp; sp.id=id; sp.pos=pos; sp.tags=tags;
    m_impl->spawnPoints.push_back(sp);
}
void SpawnManager::RegisterEntityType(uint32_t typeId, uint32_t poolSize){
    m_impl->entityTypePool[typeId]=poolSize;
}

uint32_t SpawnManager::CreateWave(const WaveDesc& desc){
    WaveRuntime wr; wr.id=m_impl->nextWaveId++; wr.desc=desc; wr.repeat=desc.repeat;
    for(auto& e:desc.entries){ WaveEntry2 e2; (WaveEntry&)e2=e; wr.state.push_back(e2); }
    m_impl->waves.push_back(wr); return wr.id;
}

void SpawnManager::StartWave(uint32_t id){
    auto* w=m_impl->FindWave(id); if(!w) return;
    w->active=true; w->complete=false; w->elapsed=0;
    for(auto& e:w->state){ e.elapsed=0; e.spawned=0; e.started=false; }
}
void SpawnManager::StopWave(uint32_t id){
    auto* w=m_impl->FindWave(id); if(w){ w->active=false; }
}
bool SpawnManager::IsWaveComplete(uint32_t id) const {
    auto* w=m_impl->FindWave(id); return w&&w->complete;
}

void SpawnManager::Tick(float dt){
    for(auto& wr:m_impl->waves){
        if(!wr.active||wr.complete) continue;
        wr.elapsed+=dt;
        bool allDone=true;
        for(auto& e:wr.state){
            if(e.spawned>=e.count){ continue; }
            allDone=false;
            if(wr.elapsed<e.delayFromWaveStart) continue;
            e.elapsed+=dt;
            if(!e.started){ e.started=true; e.elapsed=0; }
            // Spawn one entity per interval
            while(e.elapsed>=e.intervalSec && e.spawned<e.count){
                e.elapsed-=e.intervalSec;
                if(m_impl->aliveCount>=m_impl->maxConcurrent) break;
                SpawnVec3 pos=m_impl->PickSpawnPoint(e.spawnTag);
                if(m_impl->onSpawn){
                    uint32_t eid=m_impl->onSpawn(e.typeId,pos);
                    (void)eid;
                }
                e.spawned++;
                m_impl->aliveCount++;
            }
        }
        if(allDone){
            wr.complete=!wr.repeat;
            if(wr.repeat) StartWave(wr.id);
            if(wr.onComplete) wr.onComplete();
        }
    }
    // Check all waves done
    if(m_impl->onAllDone){
        bool anyActive=false;
        for(auto& w:m_impl->waves) if(w.active&&!w.complete) anyActive=true;
        if(!anyActive&&!m_impl->waves.empty()) m_impl->onAllDone();
    }
}

void SpawnManager::OnEntityDead(uint32_t /*entityId*/){
    if(m_impl->aliveCount>0) m_impl->aliveCount--;
}

void     SpawnManager::SetMaxConcurrent(uint32_t n){ m_impl->maxConcurrent=n; }
uint32_t SpawnManager::GetAliveCount()  const { return m_impl->aliveCount; }

std::vector<uint32_t> SpawnManager::GetSpawnPointsByTag(const std::string& tag) const {
    std::vector<uint32_t> out;
    for(auto& sp:m_impl->spawnPoints){
        for(auto& t:sp.tags) if(t==tag){ out.push_back(sp.id); break; }
    }
    return out;
}
SpawnVec3 SpawnManager::GetSpawnPointPos(uint32_t id) const {
    for(auto& sp:m_impl->spawnPoints) if(sp.id==id) return sp.pos;
    return {0,0,0};
}

void SpawnManager::SetOnSpawn(std::function<uint32_t(uint32_t,SpawnVec3)> cb){ m_impl->onSpawn=cb; }
void SpawnManager::SetOnWaveComplete(uint32_t id, std::function<void()> cb){
    auto* w=m_impl->FindWave(id); if(w) w->onComplete=cb;
}
void SpawnManager::SetOnAllWavesComplete(std::function<void()> cb){ m_impl->onAllDone=cb; }

} // namespace Runtime
