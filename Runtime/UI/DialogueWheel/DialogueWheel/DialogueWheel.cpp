#include "Runtime/UI/DialogueWheel/DialogueWheel/DialogueWheel.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Runtime {

static constexpr float kPi    = 3.14159265f;
static constexpr float kTwoPi = 2.f*kPi;

struct DialogueWheel::Impl {
    uint32_t maxChoices{8};
    std::vector<DialogueChoice> choices;
    int32_t hoveredIdx{-1};
    int32_t selectedIdx{-1};
    bool    visible{false};
    float   deadZone{0.2f};

    std::function<void(uint32_t)> onSelect;
    std::function<void()>         onCancel;
    std::function<void(int32_t)>  onHover;
};

DialogueWheel::DialogueWheel(): m_impl(new Impl){}
DialogueWheel::~DialogueWheel(){ Shutdown(); delete m_impl; }
void DialogueWheel::Init(uint32_t max){ m_impl->maxChoices=max; }
void DialogueWheel::Shutdown(){ m_impl->choices.clear(); }
void DialogueWheel::Reset(){ m_impl->choices.clear(); m_impl->hoveredIdx=-1; m_impl->selectedIdx=-1; m_impl->visible=false; }

void DialogueWheel::SetChoices(const std::vector<DialogueChoice>& c){
    m_impl->choices.clear();
    for(auto& ch:c){
        if((uint32_t)m_impl->choices.size()>=m_impl->maxChoices) break;
        m_impl->choices.push_back(ch);
    }
}
void DialogueWheel::AddChoice(const DialogueChoice& c){
    if((uint32_t)m_impl->choices.size()<m_impl->maxChoices) m_impl->choices.push_back(c);
}
void DialogueWheel::ClearChoices(){ m_impl->choices.clear(); m_impl->hoveredIdx=-1; }

void DialogueWheel::Update(float angleDeg, float radius){
    if(!m_impl->visible||m_impl->choices.empty()){ m_impl->hoveredIdx=-1; return; }
    if(radius<m_impl->deadZone){ m_impl->hoveredIdx=-1; return; }

    uint32_t n=(uint32_t)m_impl->choices.size();
    float sectorDeg=360.f/n;
    float normAngle=std::fmod(angleDeg+360.f,360.f);
    int32_t idx=(int32_t)(normAngle/sectorDeg)%(int32_t)n;
    if(!m_impl->choices[(uint32_t)idx].enabled){ m_impl->hoveredIdx=-1; return; }

    int32_t prev=m_impl->hoveredIdx;
    m_impl->hoveredIdx=idx;
    if(m_impl->hoveredIdx!=prev && m_impl->onHover) m_impl->onHover(m_impl->hoveredIdx);
}

int32_t DialogueWheel::GetHoveredIndex () const { return m_impl->hoveredIdx; }
int32_t DialogueWheel::GetSelectedIndex() const { return m_impl->selectedIdx; }
bool    DialogueWheel::IsVisible       () const { return m_impl->visible; }

void DialogueWheel::Confirm(){
    if(m_impl->hoveredIdx<0) return;
    m_impl->selectedIdx=m_impl->hoveredIdx;
    if(m_impl->onSelect) m_impl->onSelect((uint32_t)m_impl->selectedIdx);
    Hide();
}
void DialogueWheel::Cancel(){
    if(m_impl->onCancel) m_impl->onCancel();
    Hide();
}
void DialogueWheel::Show(){ m_impl->visible=true; }
void DialogueWheel::Hide(){ m_impl->visible=false; m_impl->hoveredIdx=-1; }

float DialogueWheel::GetSectorAngle(uint32_t idx) const {
    uint32_t n=(uint32_t)m_impl->choices.size();
    if(!n) return 0.f;
    return 360.f*idx/n;
}

DWRect DialogueWheel::GetSectorRect(uint32_t idx, float cx, float cy, float r) const {
    float angle=GetSectorAngle(idx)*kPi/180.f;
    float halfSector = kPi / (float)std::max(1u,(uint32_t)m_impl->choices.size());
    float midAngle=angle+halfSector;
    float btnR=r*0.65f;
    float bx=cx+std::cos(midAngle)*btnR;
    float by=cy+std::sin(midAngle)*btnR;
    return {bx-40,by-20,80,40};
}

void DialogueWheel::SetDeadZoneRadius(float r){ m_impl->deadZone=r; }
uint32_t DialogueWheel::GetChoiceCount() const { return (uint32_t)m_impl->choices.size(); }

void DialogueWheel::SetOnSelect(std::function<void(uint32_t)> cb){ m_impl->onSelect=cb; }
void DialogueWheel::SetOnCancel(std::function<void()> cb)        { m_impl->onCancel=cb; }
void DialogueWheel::SetOnHover (std::function<void(int32_t)> cb) { m_impl->onHover=cb; }

} // namespace Runtime
