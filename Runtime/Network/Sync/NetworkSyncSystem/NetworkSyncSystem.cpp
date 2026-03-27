#include "Runtime/Network/Sync/NetworkSyncSystem/NetworkSyncSystem.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct Snapshot {
    double    serverTime;
    SyncState state;
};

static inline float Lf(float a,float b,float t){ return a+(b-a)*t; }

static SyncState LerpState(const SyncState& a, const SyncState& b, float t){
    SyncState out;
    for(int i=0;i<3;i++) out.pos[i]=Lf(a.pos[i],b.pos[i],t);
    for(int i=0;i<4;i++) out.rot[i]=Lf(a.rot[i],b.rot[i],t); // NLerp (close enough)
    for(int i=0;i<3;i++) out.vel[i]=Lf(a.vel[i],b.vel[i],t);
    out.active=t<0.5f?a.active:b.active;
    return out;
}

struct EntityBuffer {
    std::vector<Snapshot> snapshots;
    bool desyncFired{false};
};

struct NetworkSyncSystem::Impl {
    float  interpolationDelay{0.1f};
    double renderTime{0};
    std::unordered_map<uint32_t,EntityBuffer> entities;
    std::function<void(uint32_t)> onDesync;

    EntityBuffer* Find(uint32_t id){ auto it=entities.find(id); return it!=entities.end()?&it->second:nullptr; }
    const EntityBuffer* Find(uint32_t id) const { auto it=entities.find(id); return it!=entities.end()?&it->second:nullptr; }
};

NetworkSyncSystem::NetworkSyncSystem(): m_impl(new Impl){}
NetworkSyncSystem::~NetworkSyncSystem(){ Shutdown(); delete m_impl; }

void NetworkSyncSystem::Init(float delay){ m_impl->interpolationDelay=delay; }
void NetworkSyncSystem::Shutdown(){ m_impl->entities.clear(); }
void NetworkSyncSystem::Reset(){ m_impl->entities.clear(); m_impl->renderTime=0; }
void NetworkSyncSystem::Tick(float dt){ m_impl->renderTime+=dt; }

void NetworkSyncSystem::RegisterEntity  (uint32_t id){ m_impl->entities[id]; }
void NetworkSyncSystem::UnregisterEntity(uint32_t id){ m_impl->entities.erase(id); }
bool NetworkSyncSystem::IsEntityRegistered(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void NetworkSyncSystem::PushSnapshot(uint32_t entityId, double serverTime, const SyncState& state){
    auto* eb=m_impl->Find(entityId); if(!eb) return;
    // Keep sorted by time
    Snapshot sn; sn.serverTime=serverTime; sn.state=state;
    auto it=std::lower_bound(eb->snapshots.begin(),eb->snapshots.end(),sn,
        [](const Snapshot& a,const Snapshot& b){return a.serverTime<b.serverTime;});
    eb->snapshots.insert(it,sn);
}

SyncState NetworkSyncSystem::GetInterpolatedState(uint32_t entityId, double renderTime) const {
    auto* eb=m_impl->Find(entityId); if(!eb||eb->snapshots.empty()) return SyncState{};
    double t=renderTime-m_impl->interpolationDelay;
    auto& snaps=eb->snapshots;
    // Find two surrounding snapshots
    for(size_t i=0;i+1<snaps.size();i++){
        if(snaps[i].serverTime<=t && snaps[i+1].serverTime>=t){
            double seg=snaps[i+1].serverTime-snaps[i].serverTime;
            float u=(float)((seg>0)?(t-snaps[i].serverTime)/seg:0.0);
            return LerpState(snaps[i].state,snaps[i+1].state,u);
        }
    }
    // Extrapolate if needed
    if(t<snaps.front().serverTime) return snaps.front().state;
    return GetExtrapolatedState(entityId,renderTime);
}

SyncState NetworkSyncSystem::GetExtrapolatedState(uint32_t entityId, double renderTime) const {
    auto* eb=m_impl->Find(entityId); if(!eb||eb->snapshots.empty()) return SyncState{};
    auto& last=eb->snapshots.back();
    double dt=renderTime-m_impl->interpolationDelay-last.serverTime;
    if(dt<0) dt=0;
    // Fire desync callback if significantly beyond last snapshot
    if(dt>0.2 && m_impl->onDesync && !eb->desyncFired){
        const_cast<EntityBuffer*>(eb)->desyncFired=true;
        m_impl->onDesync(entityId);
    }
    SyncState out=last.state;
    for(int i=0;i<3;i++) out.pos[i]+=last.state.vel[i]*(float)dt;
    return out;
}

void   NetworkSyncSystem::SetInterpolationDelay(float s){ m_impl->interpolationDelay=s; }
double NetworkSyncSystem::GetRenderTime()        const { return m_impl->renderTime; }

uint32_t NetworkSyncSystem::GetSnapshotCount(uint32_t id) const {
    auto* eb=m_impl->Find(id); return eb?(uint32_t)eb->snapshots.size():0;
}
void NetworkSyncSystem::PurgeOldSnapshots(double before){
    for(auto& [id,eb]:m_impl->entities){
        eb.snapshots.erase(std::remove_if(eb.snapshots.begin(),eb.snapshots.end(),
            [before](const Snapshot& s){return s.serverTime<before;}),
            eb.snapshots.end());
    }
}
void NetworkSyncSystem::SetOnDesync(std::function<void(uint32_t)> cb){ m_impl->onDesync=cb; }

} // namespace Runtime
