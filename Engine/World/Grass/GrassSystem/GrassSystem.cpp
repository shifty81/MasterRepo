#include "Engine/World/Grass/GrassSystem/GrassSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

struct GrassSystem::Impl {
    float    terrainW{100}, terrainH{100}, tileSize{1};
    uint32_t densW{0}, densH{0};
    std::vector<float> densityMap;
    // Blade
    float bladeH{0.5f}, bladeW{0.02f}, bladeBend{0.3f}, bladeTaper{0.5f};
    // Wind
    GVec3 windDir{1,0,0}; float windSpeed{2}, windTurb{0.3f};
    float windTime{0};
    // LOD
    float lodNear{20}, lodMid{50}, lodFar{100};
    // Season
    float season{0};
    // Interactions
    std::vector<GrassInteraction> interactions;
    // Instances (generated)
    std::vector<GrassInstance> allInstances;
    std::vector<GrassInstance> visibleInstances;

    static uint32_t LCG(uint32_t& s){ s=s*1664525+1013904223; return s; }
};

GrassSystem::GrassSystem(): m_impl(new Impl){}
GrassSystem::~GrassSystem(){ Shutdown(); delete m_impl; }
void GrassSystem::Init(){}
void GrassSystem::Shutdown(){ m_impl->allInstances.clear(); m_impl->visibleInstances.clear(); }
void GrassSystem::Reset(){ *m_impl=Impl{}; }

void GrassSystem::SetTerrainSize(float w, float h, float t){
    m_impl->terrainW=w; m_impl->terrainH=h; m_impl->tileSize=t;
}
void GrassSystem::SetDensityMap(const float* data, uint32_t w, uint32_t h){
    m_impl->densW=w; m_impl->densH=h;
    m_impl->densityMap.assign(data,data+w*h);
}
void GrassSystem::SetBladeParams(float height, float width, float bend, float taper){
    m_impl->bladeH=height; m_impl->bladeW=width;
    m_impl->bladeBend=bend; m_impl->bladeTaper=taper;
}
void GrassSystem::SetWindDirection (GVec3 d){ m_impl->windDir=d; }
void GrassSystem::SetWindSpeed     (float s){ m_impl->windSpeed=s; }
void GrassSystem::SetWindTurbulence(float t){ m_impl->windTurb=t; }
void GrassSystem::SetLODDistances  (float n,float m,float f){ m_impl->lodNear=n; m_impl->lodMid=m; m_impl->lodFar=f; }

void GrassSystem::GenerateInstances(){
    m_impl->allInstances.clear();
    if(m_impl->densityMap.empty()) return;
    uint32_t tilesX=(uint32_t)(m_impl->terrainW/m_impl->tileSize);
    uint32_t tilesZ=(uint32_t)(m_impl->terrainH/m_impl->tileSize);
    uint32_t seed=12345;
    for(uint32_t tz=0;tz<tilesZ;tz++) for(uint32_t tx=0;tx<tilesX;tx++){
        // Sample density
        uint32_t dx=tx*m_impl->densW/std::max(1u,tilesX);
        uint32_t dz=tz*m_impl->densH/std::max(1u,tilesZ);
        float dens=0;
        if(dx<m_impl->densW&&dz<m_impl->densH) dens=m_impl->densityMap[dz*m_impl->densW+dx];
        if(dens<=0) continue;
        // Place up to 4 blades per tile
        uint32_t count=(uint32_t)(dens*4+0.5f);
        for(uint32_t k=0;k<count;k++){
            Impl::LCG(seed);
            float ox=((seed>>16)&0xFFFF)/65535.f;
            Impl::LCG(seed);
            float oz=((seed>>16)&0xFFFF)/65535.f;
            GrassInstance gi;
            gi.pos={tx*m_impl->tileSize+ox*m_impl->tileSize,0,tz*m_impl->tileSize+oz*m_impl->tileSize};
            gi.bendAngle=0;
            gi.scale=m_impl->bladeH*(0.8f+0.4f*((seed&0xFF)/255.f));
            m_impl->allInstances.push_back(gi);
        }
    }
}

void GrassSystem::Cull(GVec3 cameraPos, GVec3, float, float){
    m_impl->visibleInstances.clear();
    for(auto& gi:m_impl->allInstances){
        float dx=gi.pos.x-cameraPos.x, dz=gi.pos.z-cameraPos.z;
        float dist=std::sqrt(dx*dx+dz*dz);
        if(dist<m_impl->lodFar) m_impl->visibleInstances.push_back(gi);
    }
}
uint32_t GrassSystem::GetVisibleInstanceCount() const { return (uint32_t)m_impl->visibleInstances.size(); }
void     GrassSystem::GetInstanceData(std::vector<GrassInstance>& out) const { out=m_impl->visibleInstances; }

void GrassSystem::AddInteraction(GVec3 pos, float r, float s){
    GrassInteraction gi; gi.pos=pos; gi.radius=r; gi.strength=s;
    m_impl->interactions.push_back(gi);
}
void GrassSystem::ClearInteractions(){ m_impl->interactions.clear(); }

void GrassSystem::SetSeason(float t){ m_impl->season=t; }
GRGB GrassSystem::GetTintColour(float t) const {
    // Summer: {0.2,0.6,0.1}  Autumn: {0.6,0.4,0.05}
    GRGB s={0.2f,0.6f,0.1f}, a={0.6f,0.4f,0.05f};
    return {s.r+(a.r-s.r)*t, s.g+(a.g-s.g)*t, s.b+(a.b-s.b)*t};
}

void GrassSystem::Tick(float dt){
    m_impl->windTime+=dt;
    float windAmp=m_impl->windSpeed*0.1f;
    // Update bend angles for visible instances with wind + interactions
    for(auto& gi:m_impl->visibleInstances){
        float wx=gi.pos.x*0.3f+m_impl->windTime*m_impl->windSpeed;
        float bend=windAmp*std::sin(wx)+m_impl->windTurb*std::cos(wx*1.7f);
        // Interactions
        for(auto& intr:m_impl->interactions){
            float dx=gi.pos.x-intr.pos.x, dz=gi.pos.z-intr.pos.z;
            float d=std::sqrt(dx*dx+dz*dz);
            if(d<intr.radius) bend-=intr.strength*(1-d/intr.radius);
        }
        gi.bendAngle=bend;
    }
}

} // namespace Engine
