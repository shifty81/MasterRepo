#include "Runtime/UI/DamageNumbers/DamageNumbers/DamageNumbers.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct DamageNumbers::Impl {
    float    lifetime   {1.5f};
    float    floatSpeed {1.5f};
    float    fadeStart  {0.6f};
    float    scalePeak  {1.4f};
    float    critScale  {1.8f};
    uint32_t poolSize   {128};
    uint32_t nextId     {1};

    std::unordered_map<uint8_t,DNColour> colourRules;
    std::vector<DamageNumberEntry> entries;

    DNColour GetColour(DamageType t) const {
        auto it=colourRules.find((uint8_t)t);
        if(it!=colourRules.end()) return it->second;
        switch(t){
            case DamageType::Critical: return {1,0.5f,0,1};
            case DamageType::Heal:     return {0.2f,1,0.3f,1};
            case DamageType::Miss:     return {0.7f,0.7f,0.7f,1};
            case DamageType::Block:    return {0.3f,0.5f,1,1};
            case DamageType::Absorb:   return {0.8f,0.3f,0.9f,1};
            default:                   return {1,1,1,1};
        }
    }
};

DamageNumbers::DamageNumbers(): m_impl(new Impl){}
DamageNumbers::~DamageNumbers(){ Shutdown(); delete m_impl; }
void DamageNumbers::Init(){}
void DamageNumbers::Shutdown(){ m_impl->entries.clear(); }
void DamageNumbers::Reset(){ m_impl->entries.clear(); m_impl->nextId=1; }
void DamageNumbers::Clear(){ m_impl->entries.clear(); }

uint32_t DamageNumbers::Spawn(DNVec3 pos, float value, DamageType type){
    // Reuse dead slot if over pool
    if(m_impl->entries.size()>=m_impl->poolSize){
        for(auto& e:m_impl->entries) if(e.age>=e.lifetime){ 
            e={m_impl->nextId++,pos,value,type,0,m_impl->lifetime,1,1,m_impl->GetColour(type)};
            return e.id;
        }
    }
    DamageNumberEntry e;
    e.id=m_impl->nextId++; e.pos=pos; e.value=value; e.type=type;
    e.age=0; e.lifetime=m_impl->lifetime; e.alpha=1; e.scale=1;
    e.colour=m_impl->GetColour(type);
    if(type==DamageType::Critical) e.scale=m_impl->critScale;
    m_impl->entries.push_back(e);
    return e.id;
}

void DamageNumbers::Tick(float dt){
    for(auto& e:m_impl->entries){
        e.age+=dt;
        // Float upward
        e.pos.y+=m_impl->floatSpeed*dt;
        // Scale bell curve: peak at midpoint
        float t=e.age/e.lifetime;
        float peakT=0.3f;
        float scaleFactor=(t<peakT)?(1+(m_impl->scalePeak-1)*(t/peakT))
                                    :(m_impl->scalePeak-(m_impl->scalePeak-1)*((t-peakT)/(1-peakT)));
        e.scale=std::max(0.f,scaleFactor);
        if(e.type==DamageType::Critical) e.scale*=m_impl->critScale;
        // Fade
        float fadeT=m_impl->fadeStart;
        e.alpha=(t<fadeT)?1.f:std::max(0.f,1.f-(t-fadeT)/(1.f-fadeT));
    }
    // Remove expired
    m_impl->entries.erase(std::remove_if(m_impl->entries.begin(),m_impl->entries.end(),
        [](const DamageNumberEntry& e){return e.age>=e.lifetime;}),m_impl->entries.end());
}

void DamageNumbers::Render(std::function<void(const DamageNumberEntry&)> drawCb) const {
    if(!drawCb) return;
    for(auto& e:m_impl->entries) drawCb(e);
}

void DamageNumbers::SetLifetime    (float s){ m_impl->lifetime=s; }
void DamageNumbers::SetFloatSpeed  (float v){ m_impl->floatSpeed=v; }
void DamageNumbers::SetFadeStartTime(float t){ m_impl->fadeStart=t; }
void DamageNumbers::SetScalePeak   (float s){ m_impl->scalePeak=s; }
void DamageNumbers::SetCriticalScale(float m){ m_impl->critScale=m; }
void DamageNumbers::SetColourRule  (DamageType t, DNColour col){ m_impl->colourRules[(uint8_t)t]=col; }
void DamageNumbers::SetPoolSize    (uint32_t n){ m_impl->poolSize=n; }

uint32_t DamageNumbers::GetActiveCount() const { return (uint32_t)m_impl->entries.size(); }

} // namespace Runtime
