#include "Runtime/Scene/AsyncScene/AsyncSceneLoader/AsyncSceneLoader.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Runtime {

enum class LoadState { Pending, Loading, Complete, Cancelled };

struct LoadJob {
    LoadHandle  handle;
    std::string path;
    int         priority;
    LoadState   state{LoadState::Pending};
    float       progress{0.f};
    SceneDesc   scene;
    std::function<void(LoadHandle)> onComplete;
};

struct AsyncSceneLoader::Impl {
    uint32_t nextHandle{1};
    std::unordered_map<uint32_t,LoadJob> jobs;
    std::vector<std::string> prefetchCache;
    SceneDesc activeScene;
    bool hasActive{false};
    std::function<void(const SceneDesc&)> onSwapIn;

    // Simulate incremental load progress per Tick
    void AdvanceJob(LoadJob& j, float dt){
        if(j.state!=LoadState::Loading) return;
        j.progress=std::min(1.f, j.progress+dt*0.5f); // 2s load time stub
        if(j.progress>=1.f){
            j.state=LoadState::Complete;
            j.scene.loaded=true;
            j.scene.path=j.path;
            j.scene.entityCount=42; // stub entity count
            if(j.onComplete) j.onComplete(j.handle);
        }
    }
};

AsyncSceneLoader::AsyncSceneLoader(): m_impl(new Impl){}
AsyncSceneLoader::~AsyncSceneLoader(){ Shutdown(); delete m_impl; }
void AsyncSceneLoader::Init(){}
void AsyncSceneLoader::Shutdown(){
    m_impl->jobs.clear();
    m_impl->prefetchCache.clear();
}

void AsyncSceneLoader::Tick(float dt){
    for(auto& [h,j]: m_impl->jobs){
        if(j.state==LoadState::Pending) j.state=LoadState::Loading;
        m_impl->AdvanceJob(j,dt);
    }
}

LoadHandle AsyncSceneLoader::LoadAsync(const std::string& path, int priority){
    LoadHandle h=m_impl->nextHandle++;
    LoadJob j; j.handle=h; j.path=path; j.priority=priority;
    m_impl->jobs[h]=j;
    return h;
}

void AsyncSceneLoader::Cancel(LoadHandle h){
    auto it=m_impl->jobs.find(h);
    if(it!=m_impl->jobs.end() && it->second.state!=LoadState::Complete)
        it->second.state=LoadState::Cancelled;
}

float AsyncSceneLoader::GetProgress  (LoadHandle h) const {
    auto it=m_impl->jobs.find(h); return it!=m_impl->jobs.end()?it->second.progress:0.f;
}
bool  AsyncSceneLoader::IsComplete   (LoadHandle h) const {
    auto it=m_impl->jobs.find(h); return it!=m_impl->jobs.end()&&it->second.state==LoadState::Complete;
}
bool  AsyncSceneLoader::IsCancelled  (LoadHandle h) const {
    auto it=m_impl->jobs.find(h); return it!=m_impl->jobs.end()&&it->second.state==LoadState::Cancelled;
}
bool  AsyncSceneLoader::IsLoading    (LoadHandle h) const {
    auto it=m_impl->jobs.find(h); return it!=m_impl->jobs.end()&&it->second.state==LoadState::Loading;
}

bool AsyncSceneLoader::SwapIn(LoadHandle h){
    auto it=m_impl->jobs.find(h);
    if(it==m_impl->jobs.end()||it->second.state!=LoadState::Complete) return false;
    m_impl->activeScene=it->second.scene;
    m_impl->hasActive=true;
    if(m_impl->onSwapIn) m_impl->onSwapIn(m_impl->activeScene);
    m_impl->jobs.erase(it);
    return true;
}

void AsyncSceneLoader::PrefetchAsset(const std::string& path){
    m_impl->prefetchCache.push_back(path);
}

void AsyncSceneLoader::SetOnComplete(LoadHandle h, std::function<void(LoadHandle)> cb){
    auto it=m_impl->jobs.find(h); if(it!=m_impl->jobs.end()) it->second.onComplete=cb;
}
void AsyncSceneLoader::SetOnSwapIn(std::function<void(const SceneDesc&)> cb){ m_impl->onSwapIn=cb; }
const SceneDesc* AsyncSceneLoader::GetActiveScene() const {
    return m_impl->hasActive ? &m_impl->activeScene : nullptr;
}

} // namespace Runtime
