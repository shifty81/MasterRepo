#include "Engine/Lod/LODManager/LODManager.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct LODManager::Impl {
    LODSettings settings;
    std::unordered_map<uint32_t,LODObject> objects;
    uint32_t nextId{1};
    float    camPos[3]{};
    float    camFov{60.f};
    std::function<void(uint32_t,int,int)> onChanged;

    float CalcDist(const float objPos[3]) const {
        float dx=objPos[0]-camPos[0],dy=objPos[1]-camPos[1],dz=objPos[2]-camPos[2];
        return std::sqrt(dx*dx+dy*dy+dz*dz);
    }
    int ChooseLOD(const LODObject& obj) const {
        float dist=CalcDist(obj.position)*settings.globalBias;
        int best=(int)obj.levels.size()-1;
        for(int i=(int)obj.levels.size()-1;i>=0;--i)
            if(dist>=obj.levels[i].startDistance/settings.globalBias) { best=i; break; }
        // Clamp
        if(best<0) best=0;
        if(best>=(int)obj.levels.size()) best=(int)obj.levels.size()-1;
        return best;
    }
};

LODManager::LODManager() : m_impl(new Impl()) {}
LODManager::~LODManager() { delete m_impl; }
void LODManager::Init(const LODSettings& s){ m_impl->settings=s; }
void LODManager::Shutdown(){ m_impl->objects.clear(); }

uint32_t LODManager::RegisterObject(const std::string& name,
                                     const std::vector<LODLevel>& levels,
                                     const float pos[3]) {
    uint32_t id=m_impl->nextId++;
    LODObject& o=m_impl->objects[id];
    o.id=id; o.name=name; o.levels=levels;
    if(pos) for(int i=0;i<3;++i) o.position[i]=pos[i];
    return id;
}
void LODManager::UnregisterObject(uint32_t id){ m_impl->objects.erase(id); }
void LODManager::SetObjectPosition(uint32_t id, const float pos[3]){
    auto it=m_impl->objects.find(id); if(it!=m_impl->objects.end()) for(int i=0;i<3;++i) it->second.position[i]=pos[i];
}
void LODManager::SetObjectActive(uint32_t id, bool a){
    auto it=m_impl->objects.find(id); if(it!=m_impl->objects.end()) it->second.active=a;
}
LODObject LODManager::GetObject(uint32_t id) const{
    auto it=m_impl->objects.find(id); return it!=m_impl->objects.end()?it->second:LODObject{};
}
int LODManager::GetCurrentLOD(uint32_t id) const{
    auto it=m_impl->objects.find(id); return it!=m_impl->objects.end()?it->second.currentLevel:0;
}
float LODManager::GetCrossFadeAlpha(uint32_t id) const{
    auto it=m_impl->objects.find(id); return it!=m_impl->objects.end()?it->second.crossFadeAlpha:1.f;
}
std::vector<LODObject> LODManager::AllObjects() const{
    std::vector<LODObject> out; for(auto&[k,v]:m_impl->objects) out.push_back(v); return out;
}
void LODManager::SetCameraPosition(const float p[3]){ for(int i=0;i<3;++i) m_impl->camPos[i]=p[i]; }
void LODManager::SetCameraFOV(float f){ m_impl->camFov=f; }
void LODManager::SetGlobalBias(float b){ m_impl->settings.globalBias=std::max(0.01f,b); }
void LODManager::SetSettings(const LODSettings& s){ m_impl->settings=s; }

void LODManager::Update(float dt){
    for(auto&[id,obj]:m_impl->objects){
        if(!obj.active) continue;
        int newLOD=m_impl->ChooseLOD(obj);
        if(newLOD!=obj.currentLevel){
            int old=obj.currentLevel; obj.currentLevel=newLOD;
            obj.crossFadeAlpha=m_impl->settings.crossFadeTime>0.f?0.f:1.f;
            if(m_impl->onChanged) m_impl->onChanged(id,old,newLOD);
        }
        // Cross-fade blend in
        if(obj.crossFadeAlpha<1.f){
            obj.crossFadeAlpha+=dt/std::max(0.01f,m_impl->settings.crossFadeTime);
            if(obj.crossFadeAlpha>1.f) obj.crossFadeAlpha=1.f;
        }
    }
}

void LODManager::OnLODChanged(std::function<void(uint32_t,int,int)> cb){ m_impl->onChanged=std::move(cb); }

} // namespace Engine
