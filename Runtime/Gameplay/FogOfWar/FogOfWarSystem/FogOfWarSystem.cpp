#include "Runtime/Gameplay/FogOfWar/FogOfWarSystem/FogOfWarSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Runtime {

struct Observer { float x{0}, y{0}, radius{5.f}; };

struct FogOfWarSystem::Impl {
    uint32_t gw{0}, gh{0};
    float cellSize{1.f};
    float decayRate{0.f};
    float falloffExp{2.f};
    std::vector<float> revealed;
    std::vector<float> explored;
    std::unordered_map<uint32_t, Observer> observers;

    int WorldToCell(float w) const { return (int)(w/cellSize); }

    void CircleFill(float cx, float cy, float radius, float value){
        int rx=(int)std::ceil(radius/cellSize);
        int ocx=WorldToCell(cx), ocy=WorldToCell(cy);
        for(int dy=-rx;dy<=rx;dy++) for(int dx=-rx;dx<=rx;dx++){
            int px=ocx+dx, py=ocy+dy;
            if(px<0||py<0||(uint32_t)px>=gw||(uint32_t)py>=gh) continue;
            float dist=std::sqrt((float)(dx*dx+dy*dy))*cellSize;
            if(dist<=radius){
                float fade=1.f-std::pow(dist/radius,falloffExp);
                float& cell=revealed[(uint32_t)py*gw+(uint32_t)px];
                cell=std::max(cell, value*fade);
            }
        }
    }
};

FogOfWarSystem::FogOfWarSystem(): m_impl(new Impl){}
FogOfWarSystem::~FogOfWarSystem(){ Shutdown(); delete m_impl; }

void FogOfWarSystem::Init(uint32_t gw, uint32_t gh, float cs){
    m_impl->gw=gw; m_impl->gh=gh; m_impl->cellSize=cs;
    m_impl->revealed.assign(gw*gh, 0.f);
    m_impl->explored.assign(gw*gh, 0.f);
}
void FogOfWarSystem::Shutdown(){ m_impl->revealed.clear(); m_impl->explored.clear(); m_impl->observers.clear(); }
void FogOfWarSystem::Reset(){ if(m_impl->gw) Init(m_impl->gw,m_impl->gh,m_impl->cellSize); m_impl->observers.clear(); }

void FogOfWarSystem::RegisterObserver  (uint32_t id, float r){ m_impl->observers[id]={0,0,r}; }
void FogOfWarSystem::UnregisterObserver(uint32_t id){ m_impl->observers.erase(id); }
void FogOfWarSystem::SetObserverPos    (uint32_t id, float x, float y){
    auto it=m_impl->observers.find(id);
    if(it!=m_impl->observers.end()){ it->second.x=x; it->second.y=y; }
}

void FogOfWarSystem::Update(float dt){
    // Decay existing revealed
    if(m_impl->decayRate>0){
        float fade=m_impl->decayRate*dt;
        for(auto& v : m_impl->revealed) v=std::max(0.f, v-fade);
    } else {
        std::fill(m_impl->revealed.begin(), m_impl->revealed.end(), 0.f);
    }
    // Reveal from each observer
    for(auto& [id,obs]:m_impl->observers)
        m_impl->CircleFill(obs.x, obs.y, obs.radius, 1.f);
    // Update explored (max of revealed)
    for(uint32_t i=0;i<(uint32_t)m_impl->explored.size();i++)
        m_impl->explored[i]=std::max(m_impl->explored[i], m_impl->revealed[i]);
}

bool FogOfWarSystem::IsVisible(float wx, float wy) const {
    int cx=m_impl->WorldToCell(wx), cy=m_impl->WorldToCell(wy);
    if(cx<0||cy<0||(uint32_t)cx>=m_impl->gw||(uint32_t)cy>=m_impl->gh) return false;
    return m_impl->revealed[(uint32_t)cy*m_impl->gw+(uint32_t)cx]>0.01f;
}
bool FogOfWarSystem::IsExplored(float wx, float wy) const {
    int cx=m_impl->WorldToCell(wx), cy=m_impl->WorldToCell(wy);
    if(cx<0||cy<0||(uint32_t)cx>=m_impl->gw||(uint32_t)cy>=m_impl->gh) return false;
    return m_impl->explored[(uint32_t)cy*m_impl->gw+(uint32_t)cx]>0.01f;
}
const float* FogOfWarSystem::GetRevealedGrid()  const { return m_impl->revealed.data(); }
const float* FogOfWarSystem::GetExploredGrid()  const { return m_impl->explored.data(); }
uint32_t     FogOfWarSystem::GridWidth()        const { return m_impl->gw; }
uint32_t     FogOfWarSystem::GridHeight()       const { return m_impl->gh; }
float        FogOfWarSystem::CellSize()         const { return m_impl->cellSize; }

void FogOfWarSystem::RevealArea(float wx, float wy, float r){
    m_impl->CircleFill(wx,wy,r,1.f);
    for(uint32_t i=0;i<(uint32_t)m_impl->explored.size();i++)
        m_impl->explored[i]=std::max(m_impl->explored[i], m_impl->revealed[i]);
}
void FogOfWarSystem::SetDecayRate(float r)       { m_impl->decayRate=r; }
void FogOfWarSystem::SetFalloffExponent(float e) { m_impl->falloffExp=e; }

} // namespace Runtime
