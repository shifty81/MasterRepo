#include "Engine/Scripting/HotReload/ScriptHotReload/ScriptHotReload.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <unordered_map>
#include <vector>

namespace Engine {

static uint64_t FileModTime(const std::string& path){
#ifdef _WIN32
    return 0; // Stub on Windows
#else
    struct { long tv_sec{}; long tv_nsec{}; } st{};
    // Use stat via stdio; avoid direct sys headers for portability
    (void)path; return 0; // stub
#endif
}

struct WatchEntry {
    std::string       path;
    uint64_t          lastMod{0};
    uint32_t          version{0};
    HotReloadCallback cb;
    bool              dir{false};
    std::string       ext;
};

struct WatchGroup {
    std::string       name;
    std::vector<std::string> paths;
    HotReloadCallback cb;
};

struct ScriptHotReload::Impl {
    std::vector<WatchEntry> entries;
    std::vector<WatchGroup> groups;
    HotReloadCallback globalCb;
    float pollInterval{0.5f};
    float elapsed{0.f};
    bool  paused{false};

    WatchEntry* Find(const std::string& path){
        for(auto& e:entries) if(e.path==path) return &e; return nullptr;
    }
};

ScriptHotReload::ScriptHotReload()  : m_impl(new Impl){}
ScriptHotReload::~ScriptHotReload() { Shutdown(); delete m_impl; }
void ScriptHotReload::Init()     {}
void ScriptHotReload::Shutdown() { m_impl->entries.clear(); }

void ScriptHotReload::Watch(const std::string& path, HotReloadCallback cb){
    if(m_impl->Find(path)) return;
    WatchEntry e; e.path=path; e.cb=cb; e.lastMod=FileModTime(path); e.version=0;
    m_impl->entries.push_back(e);
}
void ScriptHotReload::Unwatch(const std::string& path){
    auto& v=m_impl->entries;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& e){return !e.dir&&e.path==path;}),v.end());
}
void ScriptHotReload::WatchDir(const std::string& dir, const std::string& ext, HotReloadCallback cb){
    WatchEntry e; e.path=dir; e.ext=ext; e.cb=cb; e.dir=true; e.lastMod=0; e.version=0;
    m_impl->entries.push_back(e);
}
void ScriptHotReload::UnwatchDir(const std::string& dir){
    auto& v=m_impl->entries;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& e){return e.dir&&e.path==dir;}),v.end());
}
void ScriptHotReload::SetGlobalCallback(HotReloadCallback cb){ m_impl->globalCb=cb; }

void ScriptHotReload::CreateGroup(const std::string& name){
    for(auto& g:m_impl->groups) if(g.name==name) return;
    WatchGroup wg; wg.name=name; m_impl->groups.push_back(wg);
}
void ScriptHotReload::AddToGroup(const std::string& name, const std::string& path){
    for(auto& g:m_impl->groups) if(g.name==name){ g.paths.push_back(path); return; }
}
void ScriptHotReload::SetGroupCallback(const std::string& name, HotReloadCallback cb){
    for(auto& g:m_impl->groups) if(g.name==name){ g.cb=cb; return; }
}

void ScriptHotReload::SetPollInterval(float s){ m_impl->pollInterval=s; }
void ScriptHotReload::Pause  (){ m_impl->paused=true; }
void ScriptHotReload::Resume (){ m_impl->paused=false; }
bool ScriptHotReload::IsPaused() const { return m_impl->paused; }

void ScriptHotReload::ForceReload(const std::string& path){
    auto* e=m_impl->Find(path); if(!e) return;
    e->version++;
    if(e->cb) e->cb(path);
    if(m_impl->globalCb) m_impl->globalCb(path);
    // Notify groups
    for(auto& g:m_impl->groups){
        for(auto& p:g.paths) if(p==path){ if(g.cb) g.cb(path); break; }
    }
}

uint32_t ScriptHotReload::GetVersion(const std::string& path) const {
    const auto* e=m_impl->Find(path); return e?e->version:0;
}
bool ScriptHotReload::HasChanged(const std::string& path) const {
    const auto* e=m_impl->Find(path); if(!e) return false;
    return FileModTime(path)!=e->lastMod;
}

void ScriptHotReload::ClearAll(){ m_impl->entries.clear(); m_impl->groups.clear(); }

std::vector<std::string> ScriptHotReload::GetWatchedPaths() const {
    std::vector<std::string> out; for(auto& e:m_impl->entries) out.push_back(e.path); return out;
}

void ScriptHotReload::Tick(float dt)
{
    if(m_impl->paused) return;
    m_impl->elapsed+=dt;
    if(m_impl->elapsed<m_impl->pollInterval) return;
    m_impl->elapsed=0.f;

    for(auto& e:m_impl->entries){
        if(e.dir) continue; // directory watching requires platform-specific impl
        uint64_t mod=FileModTime(e.path);
        if(mod!=e.lastMod&&mod!=0){
            e.lastMod=mod; e.version++;
            if(e.cb) e.cb(e.path);
            if(m_impl->globalCb) m_impl->globalCb(e.path);
            for(auto& g:m_impl->groups)
                for(auto& p:g.paths) if(p==e.path){ if(g.cb) g.cb(e.path); break; }
        }
    }
}

} // namespace Engine
