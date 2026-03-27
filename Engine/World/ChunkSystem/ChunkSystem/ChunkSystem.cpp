#include "Engine/World/ChunkSystem/ChunkSystem/ChunkSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct ChunkHash {
    size_t operator()(const ChunkCoord& c) const {
        size_t h = (size_t)(c.x*73856093) ^ (size_t)(c.y*19349663) ^ (size_t)(c.z*83492791);
        return h;
    }
};

struct ChunkSystem::Impl {
    ChunkSystemConfig cfg;
    float focusPos[3]{};
    std::unordered_map<size_t, ChunkInfo> chunks;
    std::function<void(const ChunkCoord&)> onLoad;
    std::function<void(const ChunkCoord&)> onUnload;
    std::function<void(const ChunkCoord&, ChunkLOD)> onLOD;

    static size_t Hash(const ChunkCoord& c) { return ChunkHash{}(c); }

    ChunkInfo* Find(const ChunkCoord& c) {
        auto it=chunks.find(Hash(c)); return it!=chunks.end()?&it->second:nullptr;
    }
    const ChunkInfo* Find(const ChunkCoord& c) const {
        auto it=chunks.find(Hash(c)); return it!=chunks.end()?&it->second:nullptr;
    }

    ChunkLOD ComputeLOD(float dist) const {
        if (dist<cfg.lodDistances[0]) return ChunkLOD::High;
        if (dist<cfg.lodDistances[1]) return ChunkLOD::Medium;
        if (dist<cfg.lodDistances[2]) return ChunkLOD::Low;
        return ChunkLOD::VeryLow;
    }
};

ChunkSystem::ChunkSystem()  : m_impl(new Impl) {}
ChunkSystem::~ChunkSystem() { Shutdown(); delete m_impl; }

void ChunkSystem::Init(const ChunkSystemConfig& cfg)
{ m_impl->cfg = cfg; }
void ChunkSystem::Shutdown() { m_impl->chunks.clear(); }

void ChunkSystem::SetFocusPoint(const float pos[3])
{ for(int i=0;i<3;i++) m_impl->focusPos[i]=pos[i]; }

void ChunkSystem::SetLoadRadius  (uint32_t r) { m_impl->cfg.loadRadius=r; }
void ChunkSystem::SetUnloadRadius(uint32_t r) { m_impl->cfg.unloadRadius=r; }

ChunkCoord ChunkSystem::WorldToChunk(const float worldPos[3]) const
{
    ChunkCoord c;
    c.x = (int32_t)std::floor(worldPos[0]/m_impl->cfg.chunkSizeX);
    c.y = (int32_t)std::floor(worldPos[1]/m_impl->cfg.chunkSizeY);
    c.z = (int32_t)std::floor(worldPos[2]/m_impl->cfg.chunkSizeZ);
    return c;
}

void ChunkSystem::ChunkToWorld(const ChunkCoord& c, float out[3]) const
{
    out[0]=c.x*m_impl->cfg.chunkSizeX;
    out[1]=c.y*m_impl->cfg.chunkSizeY;
    out[2]=c.z*m_impl->cfg.chunkSizeZ;
}

const ChunkInfo* ChunkSystem::GetChunk   (const ChunkCoord& c) const { return m_impl->Find(c); }
ChunkInfo*       ChunkSystem::GetChunkMut(const ChunkCoord& c)       { return m_impl->Find(c); }
bool             ChunkSystem::IsLoaded   (const ChunkCoord& c) const {
    const auto* ci=m_impl->Find(c); return ci&&ci->state==ChunkState::Loaded;
}
ChunkState ChunkSystem::GetState(const ChunkCoord& c) const {
    const auto* ci=m_impl->Find(c); return ci?ci->state:ChunkState::Unloaded;
}

std::vector<ChunkCoord> ChunkSystem::GetLoadedChunks() const {
    std::vector<ChunkCoord> out;
    for (auto& [h,ci] : m_impl->chunks) if(ci.state==ChunkState::Loaded) out.push_back(ci.coord);
    return out;
}

std::vector<ChunkCoord> ChunkSystem::GetChunksInRadius(const float pos[3], uint32_t radius) const {
    ChunkCoord centre = WorldToChunk(pos);
    std::vector<ChunkCoord> out;
    int32_t r=(int32_t)radius;
    for(int32_t dx=-r;dx<=r;dx++) for(int32_t dz=-r;dz<=r;dz++) {
        ChunkCoord c{centre.x+dx, centre.y, centre.z+dz};
        out.push_back(c);
    }
    return out;
}

ChunkCoord ChunkSystem::GetNeighbour(const ChunkCoord& c, int32_t dx, int32_t dy, int32_t dz) const
{ return ChunkCoord{c.x+dx, c.y+dy, c.z+dz}; }

void ChunkSystem::RequestLoad(const ChunkCoord& c) {
    auto* ci = m_impl->Find(c);
    if (ci && ci->state!=ChunkState::Unloaded) return;
    if ((uint32_t)m_impl->chunks.size() >= m_impl->cfg.budget) return;
    ChunkInfo info;
    info.coord=c; info.state=ChunkState::Loaded;
    float cw[3]; ChunkToWorld(c, cw);
    float dx=cw[0]-m_impl->focusPos[0], dz=cw[2]-m_impl->focusPos[2];
    info.distFromFocus=std::sqrt(dx*dx+dz*dz);
    info.lod = m_impl->ComputeLOD(info.distFromFocus);
    m_impl->chunks[Impl::Hash(c)] = info;
    if (m_impl->onLoad) m_impl->onLoad(c);
}

void ChunkSystem::RequestUnload(const ChunkCoord& c) {
    auto it = m_impl->chunks.find(Impl::Hash(c));
    if (it==m_impl->chunks.end()) return;
    if (m_impl->onUnload) m_impl->onUnload(c);
    m_impl->chunks.erase(it);
}

void ChunkSystem::SetUserData(const ChunkCoord& c, void* d) { auto* ci=m_impl->Find(c); if(ci) ci->userData=d; }
void* ChunkSystem::GetUserData(const ChunkCoord& c) const { const auto* ci=m_impl->Find(c); return ci?ci->userData:nullptr; }
void  ChunkSystem::MarkDirty(const ChunkCoord& c) { auto* ci=m_impl->Find(c); if(ci) ci->dirty=true; }

uint32_t ChunkSystem::LoadedCount() const {
    uint32_t n=0; for(auto& [h,ci]:m_impl->chunks) if(ci.state==ChunkState::Loaded) n++; return n;
}
uint32_t ChunkSystem::QueuedCount() const {
    uint32_t n=0; for(auto& [h,ci]:m_impl->chunks) if(ci.state==ChunkState::Queued) n++; return n;
}
uint32_t ChunkSystem::BudgetUsed() const { return (uint32_t)m_impl->chunks.size(); }

void ChunkSystem::SetOnLoad   (ChunkCb cb) { m_impl->onLoad=cb; }
void ChunkSystem::SetOnUnload (ChunkCb cb) { m_impl->onUnload=cb; }
void ChunkSystem::SetOnLODChange(std::function<void(const ChunkCoord&,ChunkLOD)> cb) { m_impl->onLOD=cb; }

void ChunkSystem::Tick(float /*dt*/)
{
    ChunkCoord focusChunk = WorldToChunk(m_impl->focusPos);
    int32_t lr = (int32_t)m_impl->cfg.loadRadius;
    int32_t ur = (int32_t)m_impl->cfg.unloadRadius;

    // Load nearby
    for(int32_t dx=-lr;dx<=lr;dx++) for(int32_t dz=-lr;dz<=lr;dz++) {
        ChunkCoord c{focusChunk.x+dx, focusChunk.y, focusChunk.z+dz};
        if (!m_impl->Find(c)) RequestLoad(c);
    }
    // Unload distant
    std::vector<ChunkCoord> toUnload;
    for (auto& [h,ci] : m_impl->chunks) {
        float cw[3]; ChunkToWorld(ci.coord, cw);
        float dx=cw[0]-m_impl->focusPos[0], dz=cw[2]-m_impl->focusPos[2];
        float dist=std::sqrt(dx*dx+dz*dz);
        float threshold=ur*m_impl->cfg.chunkSizeX;
        if (dist > threshold) toUnload.push_back(ci.coord);
    }
    for (auto& c : toUnload) RequestUnload(c);

    // Update LOD
    for (auto& [h,ci] : m_impl->chunks) {
        float cw[3]; ChunkToWorld(ci.coord, cw);
        float dx2=cw[0]-m_impl->focusPos[0], dz2=cw[2]-m_impl->focusPos[2];
        ci.distFromFocus=std::sqrt(dx2*dx2+dz2*dz2);
        auto newLOD = m_impl->ComputeLOD(ci.distFromFocus);
        if (newLOD!=ci.lod && m_impl->onLOD) m_impl->onLOD(ci.coord, newLOD);
        ci.lod = newLOD;
    }
}

} // namespace Engine
