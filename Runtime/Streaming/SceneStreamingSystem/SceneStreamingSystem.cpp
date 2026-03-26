#include "Runtime/Streaming/SceneStreamingSystem/SceneStreamingSystem.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct CellCoordHash {
    size_t operator()(const CellCoord& c) const {
        return std::hash<int64_t>()(((int64_t)(uint32_t)c.x<<32)|(uint32_t)c.z);
    }
};

struct LoadJob {
    CellCoord coord;
    std::string assetPath;
};

struct SceneStreamingSystem::Impl {
    StreamingConfig config;
    std::unordered_map<CellCoord,StreamingCell,CellCoordHash> cells;

    float playerPos[3]{};
    uint32_t activeJobs{0};

    std::vector<LoadJob> pendingLoads;
    std::vector<CellCoord> pendingUnloads;

    uint64_t usedMemMB{0};

    std::function<void(CellCoord,const std::string&)> onLoaded;
    std::function<void(CellCoord)>                    onUnloaded;
    std::function<void(uint64_t,uint64_t)>            onBudget;

    float CellDist(const CellCoord& c) const {
        float cx=(c.x+0.5f)*config.cellSize;
        float cz=(c.z+0.5f)*config.cellSize;
        float dx=cx-playerPos[0], dz=cz-playerPos[2];
        return std::sqrt(dx*dx+dz*dz);
    }

    void SimulateLoad(StreamingCell& cell){
        cell.state=CellState::Loaded;
        cell.memoryUsageMB=4; // simulated 4 MB per cell
        usedMemMB+=cell.memoryUsageMB;
    }
    void SimulateUnload(StreamingCell& cell){
        usedMemMB-=std::min(cell.memoryUsageMB,usedMemMB);
        cell.memoryUsageMB=0;
        cell.state=CellState::Unloaded;
    }
};

SceneStreamingSystem::SceneStreamingSystem() : m_impl(new Impl()) {}
SceneStreamingSystem::~SceneStreamingSystem() { Shutdown(); delete m_impl; }

void SceneStreamingSystem::Init(const StreamingConfig& cfg){ m_impl->config=cfg; }
void SceneStreamingSystem::Shutdown(){ UnloadAll(); }

void SceneStreamingSystem::RegisterCell(CellCoord coord, const std::string& path){
    StreamingCell c; c.coord=coord; c.assetPath=path;
    m_impl->cells[coord]=c;
}
void SceneStreamingSystem::UnregisterCell(CellCoord coord){ m_impl->cells.erase(coord); }
bool SceneStreamingSystem::HasCell(CellCoord coord) const{ return m_impl->cells.count(coord)>0; }
StreamingCell SceneStreamingSystem::GetCell(CellCoord coord) const{
    auto it=m_impl->cells.find(coord); return it!=m_impl->cells.end()?it->second:StreamingCell{};
}
std::vector<StreamingCell> SceneStreamingSystem::AllCells() const{
    std::vector<StreamingCell> out; for(auto&[k,v]:m_impl->cells) out.push_back(v); return out;
}
std::vector<StreamingCell> SceneStreamingSystem::LoadedCells() const{
    std::vector<StreamingCell> out;
    for(auto&[k,v]:m_impl->cells) if(v.state==CellState::Loaded) out.push_back(v);
    return out;
}

void SceneStreamingSystem::SetPlayerPosition(const float pos[3]){
    for(int i=0;i<3;++i) m_impl->playerPos[i]=pos[i];
}

void SceneStreamingSystem::ForceLoad(CellCoord coord){
    auto it=m_impl->cells.find(coord); if(it==m_impl->cells.end()) return;
    if(it->second.state==CellState::Unloaded){
        it->second.state=CellState::Loading;
        m_impl->SimulateLoad(it->second);
        if(m_impl->onLoaded) m_impl->onLoaded(coord,it->second.assetPath);
    }
}
void SceneStreamingSystem::ForceUnload(CellCoord coord){
    auto it=m_impl->cells.find(coord); if(it==m_impl->cells.end()) return;
    if(it->second.state==CellState::Loaded){
        m_impl->SimulateUnload(it->second);
        if(m_impl->onUnloaded) m_impl->onUnloaded(coord);
    }
}
void SceneStreamingSystem::ForceLoadAll(){
    for(auto&[coord,cell]:m_impl->cells) ForceLoad(coord);
}
void SceneStreamingSystem::UnloadAll(){
    for(auto&[coord,cell]:m_impl->cells) if(cell.state==CellState::Loaded) ForceUnload(coord);
}

void SceneStreamingSystem::Update(float dt){
    for(auto&[coord,cell]:m_impl->cells){
        float dist=m_impl->CellDist(coord);
        cell.distToPlayer=dist;

        if(cell.state==CellState::Unloaded && dist<=m_impl->config.loadRadius){
            if(m_impl->activeJobs<m_impl->config.maxConcurrentLoads){
                ++m_impl->activeJobs;
                cell.state=CellState::Loading;
                m_impl->SimulateLoad(cell);
                --m_impl->activeJobs;
                if(m_impl->onLoaded) m_impl->onLoaded(coord,cell.assetPath);
                // Check budget
                if(m_impl->usedMemMB>m_impl->config.memoryBudgetMB && m_impl->onBudget)
                    m_impl->onBudget(m_impl->usedMemMB,m_impl->config.memoryBudgetMB);
            }
        }
        else if(cell.state==CellState::Loaded && dist>m_impl->config.unloadRadius){
            if(m_impl->config.softUnload){
                cell.unloadAlpha-=dt/std::max(0.01f,m_impl->config.softUnloadTime);
                if(cell.unloadAlpha<=0.f){
                    cell.unloadAlpha=1.f;
                    m_impl->SimulateUnload(cell);
                    if(m_impl->onUnloaded) m_impl->onUnloaded(coord);
                }
            } else {
                m_impl->SimulateUnload(cell);
                if(m_impl->onUnloaded) m_impl->onUnloaded(coord);
            }
        }
    }
}

uint64_t SceneStreamingSystem::TotalLoadedMemoryMB() const{ return m_impl->usedMemMB; }
uint32_t SceneStreamingSystem::LoadedCellCount()     const{
    uint32_t n=0; for(auto&[k,v]:m_impl->cells) if(v.state==CellState::Loaded) ++n; return n;
}
uint32_t SceneStreamingSystem::PendingLoadCount()    const{ return m_impl->activeJobs; }

void SceneStreamingSystem::OnCellLoaded(std::function<void(CellCoord,const std::string&)> cb){ m_impl->onLoaded=std::move(cb); }
void SceneStreamingSystem::OnCellUnloaded(std::function<void(CellCoord)> cb)                { m_impl->onUnloaded=std::move(cb); }
void SceneStreamingSystem::OnMemoryBudgetExceeded(std::function<void(uint64_t,uint64_t)> cb){ m_impl->onBudget=std::move(cb); }

} // namespace Runtime
