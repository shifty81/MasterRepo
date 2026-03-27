#include "Engine/Audio/Environment/EnvironmentSoundSystem/EnvironmentSoundSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct EnvZone {
    uint32_t      id;
    EnvZoneShape  shape{EnvZoneShape::Sphere};
    float         c[3]{};          // centre
    float         half[3]{1,1,1};  // half-extents (x=radius for sphere)
    std::string   clip;
    bool          loop{true};
    float         volume{1.f};
    uint32_t      reverb{0};
    int           priority{0};
    // runtime
    float         weight{0};
    bool          wasActive{false};
};

struct EnvironmentSoundSystem::Impl {
    std::unordered_map<uint32_t,EnvZone> zones;
    float listener[3]{};
    float crossfadeDur{1.f};
    std::function<void(uint32_t)> onEnter, onExit;

    float ComputeWeight(const EnvZone& z) const {
        float dx=listener[0]-z.c[0], dy=listener[1]-z.c[1], dz=listener[2]-z.c[2];
        if(z.shape==EnvZoneShape::Sphere){
            float r=z.half[0];
            float d=std::sqrt(dx*dx+dy*dy+dz*dz);
            return d<r?1.f-d/r:0.f;
        } else {
            // Box: weight by minimum distance to edge
            float tx=std::max(0.f,std::abs(dx)-z.half[0]);
            float ty=std::max(0.f,std::abs(dy)-z.half[1]);
            float tz=std::max(0.f,std::abs(dz)-z.half[2]);
            if(tx>0||ty>0||tz>0) return 0;
            float ex=1.f-std::abs(dx)/z.half[0];
            float ey=1.f-std::abs(dy)/z.half[1];
            float ez=1.f-std::abs(dz)/z.half[2];
            return std::min({ex,ey,ez});
        }
    }
};

EnvironmentSoundSystem::EnvironmentSoundSystem(): m_impl(new Impl){}
EnvironmentSoundSystem::~EnvironmentSoundSystem(){ Shutdown(); delete m_impl; }
void EnvironmentSoundSystem::Init(){}
void EnvironmentSoundSystem::Shutdown(){ m_impl->zones.clear(); }
void EnvironmentSoundSystem::Reset(){ m_impl->zones.clear(); }

bool EnvironmentSoundSystem::RegisterZone(uint32_t id, EnvZoneShape shape,
                                            float cx, float cy, float cz,
                                            float hw, float hh, float hd){
    if(m_impl->zones.count(id)) return false;
    EnvZone z; z.id=id; z.shape=shape;
    z.c[0]=cx; z.c[1]=cy; z.c[2]=cz;
    z.half[0]=hw; z.half[1]=hh; z.half[2]=hd;
    m_impl->zones[id]=z; return true;
}
void EnvironmentSoundSystem::RemoveZone(uint32_t id){ m_impl->zones.erase(id); }

void EnvironmentSoundSystem::SetZoneSound(uint32_t id, const std::string& clip,
                                           bool loop, float vol){
    auto it=m_impl->zones.find(id); if(it!=m_impl->zones.end()){
        it->second.clip=clip; it->second.loop=loop; it->second.volume=vol;
    }
}
void EnvironmentSoundSystem::SetZoneReverb(uint32_t id, uint32_t preset){
    auto it=m_impl->zones.find(id); if(it!=m_impl->zones.end()) it->second.reverb=preset;
}
void EnvironmentSoundSystem::SetZonePriority(uint32_t id, int p){
    auto it=m_impl->zones.find(id); if(it!=m_impl->zones.end()) it->second.priority=p;
}

void EnvironmentSoundSystem::SetListenerPosition(float x, float y, float z){
    m_impl->listener[0]=x; m_impl->listener[1]=y; m_impl->listener[2]=z;
}

void EnvironmentSoundSystem::Tick(float /*dt*/){
    for(auto& [id,z]:m_impl->zones){
        float w=m_impl->ComputeWeight(z);
        bool now=w>0, was=z.wasActive;
        z.weight=w;
        if(now&&!was&&m_impl->onEnter) m_impl->onEnter(id);
        if(!now&&was&&m_impl->onExit ) m_impl->onExit(id);
        z.wasActive=now;
    }
}

uint32_t EnvironmentSoundSystem::GetActiveZones(std::vector<uint32_t>& out) const {
    out.clear();
    for(auto& [id,z]:m_impl->zones) if(z.weight>0) out.push_back(id);
    return (uint32_t)out.size();
}
float EnvironmentSoundSystem::GetZoneWeight(uint32_t id) const {
    auto it=m_impl->zones.find(id); return it!=m_impl->zones.end()?it->second.weight:0;
}

void EnvironmentSoundSystem::SetCrossfadeDuration(float s){ m_impl->crossfadeDur=s; }
void EnvironmentSoundSystem::SetOnZoneEnter(std::function<void(uint32_t)> cb){ m_impl->onEnter=cb; }
void EnvironmentSoundSystem::SetOnZoneExit (std::function<void(uint32_t)> cb){ m_impl->onExit=cb; }

} // namespace Engine
