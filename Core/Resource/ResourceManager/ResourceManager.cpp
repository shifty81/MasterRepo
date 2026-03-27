#include "Core/Resource/ResourceManager/ResourceManager.h"
#include <algorithm>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Core {

struct CacheEntry {
    std::string path;
    std::shared_ptr<ResourceBase> res;
    uint64_t sizeBytes{0};
    bool     pending{false};
    float    lastAccess{0};
    std::vector<std::function<void(std::shared_ptr<ResourceBase>)>> cbs;
};

struct ResourceManager::Impl {
    std::unordered_map<std::string,CacheEntry> cache;
    std::unordered_map<std::string,LoaderFn>   loaders;
    uint64_t budget{0};
    float    time{0};
    ResourceHandle nextHandle{1};
    std::mutex mtx;

    std::string ExtOf(const std::string& p) const {
        auto d=p.rfind('.'); return d!=std::string::npos?p.substr(d+1):"";
    }
    uint64_t TotalBytes() const {
        uint64_t t=0; for(auto& [k,e]:cache) t+=e.sizeBytes; return t;
    }
    void EvictLRU(){
        if(!budget) return;
        while(TotalBytes()>budget&&!cache.empty()){
            // Find lowest lastAccess with no external refs
            std::string victim; float oldest=1e30f;
            for(auto& [k,e]:cache) if(!e.pending&&e.res.use_count()<=1&&e.lastAccess<oldest){ oldest=e.lastAccess; victim=k; }
            if(victim.empty()) break;
            cache.erase(victim);
        }
    }
};

ResourceManager::ResourceManager() : m_impl(new Impl){}
ResourceManager::~ResourceManager(){ Shutdown(); delete m_impl; }
void ResourceManager::Init()     {}
void ResourceManager::Shutdown() { std::unique_lock<std::mutex> lk(m_impl->mtx); m_impl->cache.clear(); }

void ResourceManager::RegisterLoader(const std::string& ext, LoaderFn fn){ m_impl->loaders[ext]=fn; }

ResourceHandle ResourceManager::LoadAsync(const std::string& path,
                                            std::function<void(std::shared_ptr<ResourceBase>)> cb){
    std::unique_lock<std::mutex> lk(m_impl->mtx);
    auto& e=m_impl->cache[path];
    e.path=path;
    if(e.res){ if(cb) cb(e.res); return m_impl->nextHandle++; }
    e.pending=true; e.cbs.push_back(cb);
    ResourceHandle h=m_impl->nextHandle++;
    // Run loader synchronously (background threading omitted for portability)
    auto ext=m_impl->ExtOf(path);
    auto it=m_impl->loaders.find(ext);
    if(it!=m_impl->loaders.end()){ e.res=it->second(path); e.sizeBytes=e.res?e.res->sizeBytes:0; }
    e.pending=false;
    for(auto& c:e.cbs) if(c) c(e.res);
    e.cbs.clear();
    m_impl->EvictLRU();
    return h;
}

std::shared_ptr<ResourceBase> ResourceManager::LoadSync(const std::string& path){
    std::shared_ptr<ResourceBase> result;
    LoadAsync(path,[&](auto r){ result=r; });
    return result;
}

std::shared_ptr<ResourceBase> ResourceManager::Get(const std::string& path) const {
    auto it=m_impl->cache.find(path); return it!=m_impl->cache.end()?it->second.res:nullptr;
}
bool ResourceManager::IsLoaded (const std::string& p) const { auto it=m_impl->cache.find(p); return it!=m_impl->cache.end()&&it->second.res&&!it->second.pending; }
bool ResourceManager::IsPending(const std::string& p) const { auto it=m_impl->cache.find(p); return it!=m_impl->cache.end()&&it->second.pending; }

void ResourceManager::Unload(const std::string& p){ std::unique_lock<std::mutex> lk(m_impl->mtx); m_impl->cache.erase(p); }
void ResourceManager::Reload (const std::string& p){ Unload(p); LoadAsync(p,nullptr); }
void ResourceManager::UnloadAll(){ std::unique_lock<std::mutex> lk(m_impl->mtx); m_impl->cache.clear(); }

void     ResourceManager::SetMemoryBudget(uint64_t b){ m_impl->budget=b; m_impl->EvictLRU(); }
uint64_t ResourceManager::GetMemoryUsage() const { return m_impl->TotalBytes(); }
void     ResourceManager::TriggerEviction(){ m_impl->EvictLRU(); }

ResourceStats ResourceManager::GetStats() const {
    ResourceStats s;
    for(auto& [k,e]:m_impl->cache){
        if(e.pending) s.pending++; else if(e.res) s.loaded++;
        s.bytes+=e.sizeBytes;
    }
    return s;
}

void ResourceManager::Tick(float dt){ m_impl->time+=dt; }
void ResourceManager::WaitAll(){}

} // namespace Core
