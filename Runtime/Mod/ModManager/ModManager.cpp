#include "Runtime/Mod/ModManager/ModManager.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Runtime {

struct ModEntry {
    ModManifest manifest;
    ModState    state{ModState::Discovered};
    std::string error;
};

struct ModManager::Impl {
    std::vector<ModEntry>                   mods;
    std::string                             modsDir;
    std::function<void(const std::string&)> onLoad;
    std::function<void(const std::string&)> onUnload;
    std::function<void(const std::string&,const std::string&)> onError;

    ModEntry* Find(const std::string& id) {
        for(auto& e:mods) if(e.manifest.id==id) return &e; return nullptr;
    }
    const ModEntry* Find(const std::string& id) const {
        for(auto& e:mods) if(e.manifest.id==id) return &e; return nullptr;
    }

    // Topological sort (Kahn's algorithm)
    std::vector<std::string> TopoSort() const {
        std::unordered_map<std::string,int32_t> inDeg;
        std::unordered_map<std::string,std::vector<std::string>> adj;
        for(auto& e:mods) {
            if(!e.manifest.enabled) continue;
            inDeg[e.manifest.id];
            for(auto& dep:e.manifest.dependencies) {
                adj[dep].push_back(e.manifest.id);
                inDeg[e.manifest.id]++;
            }
        }
        std::vector<std::string> queue, result;
        for(auto& [k,v]:inDeg) if(v==0) queue.push_back(k);
        // Sort by loadOrder hint for determinism
        std::sort(queue.begin(),queue.end(),[this](auto& a, auto& b){
            const auto* ma=Find(a); const auto* mb=Find(b);
            if(!ma||!mb) return false;
            return ma->manifest.loadOrder < mb->manifest.loadOrder;
        });
        while(!queue.empty()) {
            auto cur=queue.front(); queue.erase(queue.begin());
            result.push_back(cur);
            if(adj.count(cur)) for(auto& next:adj[cur]) {
                if(--inDeg[next]==0) queue.push_back(next);
            }
        }
        // Append any not reached (cycle / missing dep)
        for(auto& e:mods) {
            if(!e.manifest.enabled) continue;
            if(std::find(result.begin(),result.end(),e.manifest.id)==result.end())
                result.push_back(e.manifest.id);
        }
        return result;
    }
};

ModManager::ModManager()  : m_impl(new Impl) {}
ModManager::~ModManager() { Shutdown(); delete m_impl; }

void ModManager::Init(const std::string& dir) { m_impl->modsDir=dir; }
void ModManager::Shutdown()                    { m_impl->mods.clear(); }

uint32_t ModManager::Discover()
{
    // Stub: in a real implementation this would scan modsDir for manifest.json files.
    // Return 0 since we can't do real filesystem enumeration in all environments.
    return 0;
}

bool ModManager::RegisterManifest(const ModManifest& manifest)
{
    if(m_impl->Find(manifest.id)) return false; // already registered
    ModEntry e; e.manifest=manifest; e.state=ModState::Discovered;
    m_impl->mods.push_back(e); return true;
}

std::vector<std::string> ModManager::GetLoadOrder() const { return m_impl->TopoSort(); }

std::vector<std::string> ModManager::GetConflicts() const {
    std::vector<std::string> out;
    for(auto& e:m_impl->mods) {
        if(!e.manifest.enabled) continue;
        for(auto& cid:e.manifest.conflicts) {
            const auto* other=m_impl->Find(cid);
            if(other&&other->manifest.enabled) {
                out.push_back(e.manifest.id);
                out.push_back(cid);
            }
        }
    }
    return out;
}

bool ModManager::Load(const std::string& id)
{
    auto* e=m_impl->Find(id); if(!e) return false;
    if(e->state==ModState::Loaded) return true;
    // Check deps
    for(auto& dep:e->manifest.dependencies) {
        auto* de=m_impl->Find(dep);
        if(!de||de->state!=ModState::Loaded) { e->state=ModState::Error; e->error="Missing dep: "+dep; return false; }
    }
    e->state=ModState::Loaded;
    if(m_impl->onLoad) m_impl->onLoad(id);
    return true;
}

bool ModManager::Unload(const std::string& id)
{
    auto* e=m_impl->Find(id); if(!e) return false;
    e->state=ModState::Enabled;
    if(m_impl->onUnload) m_impl->onUnload(id);
    return true;
}

void ModManager::Enable(const std::string& id) {
    auto* e=m_impl->Find(id); if(e) { e->manifest.enabled=true; e->state=ModState::Enabled; }
}
void ModManager::Disable(const std::string& id) {
    auto* e=m_impl->Find(id); if(e) { e->manifest.enabled=false; e->state=ModState::Disabled; }
}
bool ModManager::IsLoaded (const std::string& id) const {
    const auto* e=m_impl->Find(id); return e&&e->state==ModState::Loaded;
}
bool ModManager::IsEnabled(const std::string& id) const {
    const auto* e=m_impl->Find(id); return e&&e->manifest.enabled;
}

const ModManifest* ModManager::GetManifest(const std::string& id) const {
    const auto* e=m_impl->Find(id); return e?&e->manifest:nullptr;
}
ModStatus ModManager::GetStatus(const std::string& id) const {
    const auto* e=m_impl->Find(id);
    if(!e) return {id, ModState::Discovered, "not found"};
    return {e->manifest.id, e->state, e->error};
}

std::vector<ModManifest> ModManager::GetAll() const {
    std::vector<ModManifest> out; for(auto& e:m_impl->mods) out.push_back(e.manifest); return out;
}
std::vector<ModManifest> ModManager::GetEnabled() const {
    std::vector<ModManifest> out;
    for(auto& e:m_impl->mods) if(e.manifest.enabled) out.push_back(e.manifest);
    return out;
}
uint32_t ModManager::ModCount() const { return (uint32_t)m_impl->mods.size(); }

bool ModManager::SaveEnabled(const std::string& path) const {
    std::ofstream f(path); if(!f) return false;
    f << "[";
    bool first=true;
    for(auto& e:m_impl->mods) if(e.manifest.enabled) {
        if(!first) f<<","; first=false;
        f<<"\""<<e.manifest.id<<"\"";
    }
    f << "]"; return true;
}

bool ModManager::LoadEnabled(const std::string& path) {
    std::ifstream f(path); if(!f) return false;
    for(auto& e:m_impl->mods) e.manifest.enabled=false;
    std::string line; std::getline(f,line);
    std::istringstream ss(line);
    char c; std::string tok;
    while(ss>>c) {
        if(c=='"') {
            tok.clear();
            while(ss>>c&&c!='"') tok+=c;
            auto* e=m_impl->Find(tok); if(e) e->manifest.enabled=true;
        }
    }
    return true;
}

void ModManager::SetOnLoad  (std::function<void(const std::string&)> cb) { m_impl->onLoad=cb; }
void ModManager::SetOnUnload(std::function<void(const std::string&)> cb) { m_impl->onUnload=cb; }
void ModManager::SetOnError (std::function<void(const std::string&,const std::string&)> cb) { m_impl->onError=cb; }

} // namespace Runtime
