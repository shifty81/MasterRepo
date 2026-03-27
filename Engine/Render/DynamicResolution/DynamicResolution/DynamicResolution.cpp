#include "Engine/Render/DynamicResolution/DynamicResolution/DynamicResolution.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace Engine {

struct DynamicResolution::Impl {
    uint32_t baseW{1920}, baseH{1080};
    float    targetFrameTime{1.f/60.f};
    float    currentScale{1.f};
    float    minScale{0.5f}, maxScale{1.f};
    float    step{0.05f};
    float    upThreshold  {0.85f};
    float    downThreshold{1.05f};
    uint32_t historyLen{8};
    std::vector<float> history;
    std::function<void(float,float)> onChanged;
};

DynamicResolution::DynamicResolution()  : m_impl(new Impl){}
DynamicResolution::~DynamicResolution() { Shutdown(); delete m_impl; }

void DynamicResolution::Init(uint32_t w, uint32_t h, float fps){
    m_impl->baseW=w; m_impl->baseH=h;
    m_impl->targetFrameTime=fps>0.f?1.f/fps:1.f/60.f;
    m_impl->currentScale=m_impl->maxScale;
    m_impl->history.clear();
}
void DynamicResolution::Shutdown(){}

void DynamicResolution::Tick(float ft)
{
    m_impl->history.push_back(ft);
    if((uint32_t)m_impl->history.size()>m_impl->historyLen)
        m_impl->history.erase(m_impl->history.begin());

    float avg=0.f;
    for(auto t:m_impl->history) avg+=t;
    avg/=(float)m_impl->history.size();

    float worst=*std::max_element(m_impl->history.begin(),m_impl->history.end());
    float old=m_impl->currentScale;

    if(worst>m_impl->targetFrameTime*m_impl->downThreshold){
        m_impl->currentScale=std::max(m_impl->minScale, m_impl->currentScale-m_impl->step);
    } else if(avg<m_impl->targetFrameTime*m_impl->upThreshold){
        m_impl->currentScale=std::min(m_impl->maxScale, m_impl->currentScale+m_impl->step);
    }

    if(m_impl->currentScale!=old && m_impl->onChanged) m_impl->onChanged(old,m_impl->currentScale);
}

void  DynamicResolution::SetScaleRange(float mn, float mx){ m_impl->minScale=mn; m_impl->maxScale=mx; }
float DynamicResolution::GetMinScale()  const{ return m_impl->minScale; }
float DynamicResolution::GetMaxScale()  const{ return m_impl->maxScale; }
void  DynamicResolution::SetScaleStep(float s){ m_impl->step=s; }
float DynamicResolution::GetScaleStep() const { return m_impl->step; }
void  DynamicResolution::SetUpThreshold  (float t){ m_impl->upThreshold=t; }
void  DynamicResolution::SetDownThreshold(float t){ m_impl->downThreshold=t; }
void  DynamicResolution::SetHistoryLength(uint32_t f){ m_impl->historyLen=f; }
uint32_t DynamicResolution::GetHistoryLength() const { return m_impl->historyLen; }
float DynamicResolution::GetCurrentScale() const { return m_impl->currentScale; }
void  DynamicResolution::ForceScale(float s){
    m_impl->currentScale=std::max(m_impl->minScale,std::min(m_impl->maxScale,s));
    m_impl->history.clear();
}
void DynamicResolution::Reset(){
    m_impl->currentScale=m_impl->maxScale; m_impl->history.clear();
}
void DynamicResolution::GetResolution(uint32_t& w, uint32_t& h) const {
    w=(uint32_t)(m_impl->baseW*m_impl->currentScale);
    h=(uint32_t)(m_impl->baseH*m_impl->currentScale);
    // Ensure even
    w=(w+1)&~1u; h=(h+1)&~1u;
}
void DynamicResolution::SetOnScaleChanged(std::function<void(float,float)> cb){ m_impl->onChanged=cb; }
float DynamicResolution::AverageFrameTime() const {
    if(m_impl->history.empty()) return 0.f;
    float s=0; for(auto t:m_impl->history) s+=t; return s/(float)m_impl->history.size();
}
float DynamicResolution::WorstFrameTime() const {
    if(m_impl->history.empty()) return 0.f;
    return *std::max_element(m_impl->history.begin(),m_impl->history.end());
}

} // namespace Engine
