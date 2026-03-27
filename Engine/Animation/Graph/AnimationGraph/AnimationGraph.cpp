#include "Engine/Animation/Graph/AnimationGraph/AnimationGraph.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct Transition {
    uint32_t    fromState;
    uint32_t    toState;
    std::string condVar;
    float       threshold;
    float       crossfadeSec;
};

struct BlendClip { float x,y; std::string clip; };

struct StateData {
    uint32_t    id;
    std::string clip;
    bool        loop{true};
    // Blend space
    bool        has1D{false}; std::string axis1D;
    bool        has2D{false}; std::string axisX, axisY;
    std::vector<BlendClip> blendClips;
    std::function<void()> onEnter, onExit;
};

struct GraphData {
    std::unordered_map<uint32_t,StateData> states;
    std::vector<Transition>                transitions;
    std::unordered_map<std::string,float>  params;
    uint32_t currentState{0};
    uint32_t prevState{0};
    float    clipTime{0};
    float    crossfadeTime{0};
    float    crossfadeDur{0};
};

struct AnimationGraph::Impl {
    std::unordered_map<uint32_t,GraphData> graphs;
    GraphData* Find(uint32_t id){ auto it=graphs.find(id); return it!=graphs.end()?&it->second:nullptr; }
};

AnimationGraph::AnimationGraph(): m_impl(new Impl){}
AnimationGraph::~AnimationGraph(){ Shutdown(); delete m_impl; }
void AnimationGraph::Init(){}
void AnimationGraph::Shutdown(){ m_impl->graphs.clear(); }
void AnimationGraph::Reset(){ m_impl->graphs.clear(); }

void AnimationGraph::CreateGraph(uint32_t id){ m_impl->graphs[id]=GraphData{}; }
void AnimationGraph::DestroyGraph(uint32_t id){ m_impl->graphs.erase(id); }

bool AnimationGraph::AddState(uint32_t gId,uint32_t sId,const std::string& clip,bool loop){
    auto* g=m_impl->Find(gId); if(!g) return false;
    StateData s; s.id=sId; s.clip=clip; s.loop=loop;
    g->states[sId]=s; return true;
}
void AnimationGraph::SetEntryState(uint32_t gId,uint32_t sId){
    auto* g=m_impl->Find(gId); if(g) g->currentState=sId;
}
void AnimationGraph::AddTransition(uint32_t gId,uint32_t from,uint32_t to,
                                    const std::string& var,float thresh,float fade){
    auto* g=m_impl->Find(gId); if(!g) return;
    Transition t; t.fromState=from; t.toState=to;
    t.condVar=var; t.threshold=thresh; t.crossfadeSec=fade;
    g->transitions.push_back(t);
}

void AnimationGraph::SetParameter(uint32_t gId,const std::string& name,float v){
    auto* g=m_impl->Find(gId); if(g) g->params[name]=v;
}
float AnimationGraph::GetParameter(uint32_t gId,const std::string& name) const {
    auto* g=m_impl->Find(gId); if(!g) return 0;
    auto it=g->params.find(name); return it!=g->params.end()?it->second:0;
}

void AnimationGraph::AddBlendSpace1D(uint32_t gId,uint32_t sId,const std::string& axis){
    auto* g=m_impl->Find(gId); if(!g) return;
    auto it=g->states.find(sId); if(it!=g->states.end()){ it->second.has1D=true; it->second.axis1D=axis; }
}
void AnimationGraph::AddBlendSpace2D(uint32_t gId,uint32_t sId,
                                      const std::string& ax,const std::string& ay){
    auto* g=m_impl->Find(gId); if(!g) return;
    auto it=g->states.find(sId); if(it!=g->states.end()){ it->second.has2D=true; it->second.axisX=ax; it->second.axisY=ay; }
}
void AnimationGraph::AddClipToBlendSpace(uint32_t gId,uint32_t sId,
                                          float x,float y,const std::string& clip){
    auto* g=m_impl->Find(gId); if(!g) return;
    auto it=g->states.find(sId); if(it!=g->states.end()) it->second.blendClips.push_back({x,y,clip});
}

void AnimationGraph::Tick(uint32_t gId, float dt){
    auto* g=m_impl->Find(gId); if(!g) return;
    // Check transitions
    auto* cs=g->states.count(g->currentState)?&g->states.at(g->currentState):nullptr;
    if(cs){
        for(auto& tr:g->transitions){
            if(tr.fromState!=g->currentState) continue;
            float pv=0; auto pit=g->params.find(tr.condVar); if(pit!=g->params.end()) pv=pit->second;
            if(pv>=tr.threshold){
                // Transition
                if(cs->onExit) cs->onExit();
                g->prevState=g->currentState;
                g->currentState=tr.toState;
                g->crossfadeTime=0; g->crossfadeDur=tr.crossfadeSec; g->clipTime=0;
                auto* ns=g->states.count(tr.toState)?&g->states.at(tr.toState):nullptr;
                if(ns&&ns->onEnter) ns->onEnter();
                break;
            }
        }
    }
    g->clipTime+=dt;
    if(g->crossfadeDur>0) g->crossfadeTime=std::min(g->crossfadeTime+dt,g->crossfadeDur);
}

uint32_t AnimationGraph::GetCurrentState(uint32_t gId) const { auto* g=m_impl->Find(gId); return g?g->currentState:0; }
float AnimationGraph::GetCurrentClipTime(uint32_t gId) const { auto* g=m_impl->Find(gId); return g?g->clipTime:0; }
float AnimationGraph::GetBlendWeight(uint32_t gId,uint32_t sId) const {
    auto* g=m_impl->Find(gId); if(!g) return 0;
    if(sId==g->currentState) return g->crossfadeDur>0?g->crossfadeTime/g->crossfadeDur:1.f;
    if(sId==g->prevState) return g->crossfadeDur>0?1.f-g->crossfadeTime/g->crossfadeDur:0.f;
    return 0;
}

void AnimationGraph::SetOnStateEnter(uint32_t gId,uint32_t sId,std::function<void()> cb){
    auto* g=m_impl->Find(gId); if(!g) return;
    auto it=g->states.find(sId); if(it!=g->states.end()) it->second.onEnter=cb;
}
void AnimationGraph::SetOnStateExit(uint32_t gId,uint32_t sId,std::function<void()> cb){
    auto* g=m_impl->Find(gId); if(!g) return;
    auto it=g->states.find(sId); if(it!=g->states.end()) it->second.onExit=cb;
}

} // namespace Engine
