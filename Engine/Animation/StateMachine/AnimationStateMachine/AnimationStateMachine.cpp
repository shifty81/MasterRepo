#include "Engine/Animation/StateMachine/AnimationStateMachine/AnimationStateMachine.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct StateData {
    uint32_t    id;
    std::string clipName;
    float       speed{1};
    bool        looping{true};
    std::function<void(uint32_t)> onEnter;
    std::function<void(uint32_t)> onExit;
};

struct TransitionData {
    uint32_t fromId, toId;
    float    blendTime{0.2f};
    std::function<bool(uint32_t)> condition;
    std::function<void(uint32_t)> onTransition;
};

struct AgentAnimState {
    uint32_t currentState{0};
    uint32_t blendFromState{0};
    float    normalizedTime{0};
    float    blendTime{0};
    float    blendProgress{0};
    std::unordered_map<std::string,bool>    bools;
    std::unordered_map<std::string,float>   floats;
    std::unordered_map<std::string,int32_t> ints;
};

struct LayerData {
    uint32_t id;
    std::vector<uint32_t> maskBones;
    float weight{1};
};

struct AnimationStateMachine::Impl {
    uint32_t defaultState{0};
    std::unordered_map<uint32_t,StateData>    states;
    std::vector<TransitionData>               transitions;
    std::vector<LayerData>                    layers;
    std::unordered_map<uint32_t,AgentAnimState> agents;

    AgentAnimState& GetAgent(uint32_t id){ return agents[id]; }
    StateData* FindState(uint32_t id){ auto it=states.find(id); return it!=states.end()?&it->second:nullptr; }
};

AnimationStateMachine::AnimationStateMachine(): m_impl(new Impl){}
AnimationStateMachine::~AnimationStateMachine(){ Shutdown(); delete m_impl; }
void AnimationStateMachine::Init(){}
void AnimationStateMachine::Shutdown(){ m_impl->states.clear(); m_impl->agents.clear(); m_impl->transitions.clear(); }

void AnimationStateMachine::AddState(uint32_t id, const std::string& clip, float spd, bool loop){
    StateData sd; sd.id=id; sd.clipName=clip; sd.speed=spd; sd.looping=loop;
    m_impl->states[id]=sd;
}
void AnimationStateMachine::AddTransition(uint32_t from, uint32_t to,
    std::function<bool(uint32_t)> cond, float blendTime){
    TransitionData td; td.fromId=from; td.toId=to; td.condition=cond; td.blendTime=blendTime;
    m_impl->transitions.push_back(td);
}
void AnimationStateMachine::SetDefaultState(uint32_t id){ m_impl->defaultState=id; }

void AnimationStateMachine::Play(uint32_t agentId, uint32_t stateId, float blendTime){
    auto& ag=m_impl->GetAgent(agentId);
    auto* from=m_impl->FindState(ag.currentState);
    if(from&&from->onExit) from->onExit(agentId);
    ag.blendFromState=ag.currentState;
    ag.currentState=stateId;
    ag.blendProgress=0; ag.blendTime=blendTime;
    ag.normalizedTime=0;
    auto* to=m_impl->FindState(stateId);
    if(to&&to->onEnter) to->onEnter(agentId);
}

AnimFrame AnimationStateMachine::Tick(uint32_t agentId, float dt){
    auto& ag=m_impl->GetAgent(agentId);
    if(ag.currentState==0&&m_impl->defaultState!=0) Play(agentId,m_impl->defaultState,0);

    auto* state=m_impl->FindState(ag.currentState);
    if(!state) return AnimFrame{};

    float spd=state->speed;
    ag.normalizedTime+=dt*spd;
    if(ag.normalizedTime>=1.f){
        if(state->looping) ag.normalizedTime=std::fmod(ag.normalizedTime,1.f);
        else ag.normalizedTime=1.f;
    }

    // Blend progress
    if(ag.blendTime>0) ag.blendProgress=std::min(1.f,ag.blendProgress+dt/ag.blendTime);
    else ag.blendProgress=1;

    // Check transitions
    for(auto& tr:m_impl->transitions){
        if(tr.fromId!=ag.currentState) continue;
        if(tr.condition&&tr.condition(agentId)){
            if(tr.onTransition) tr.onTransition(agentId);
            Play(agentId,tr.toId,tr.blendTime);
            break;
        }
    }

    AnimFrame frame;
    frame.clipName=state->clipName;
    frame.normalizedTime=ag.normalizedTime;
    frame.blendWeight=ag.blendProgress;
    frame.stateId=ag.currentState;
    return frame;
}

void AnimationStateMachine::Reset(uint32_t agentId){
    auto& ag=m_impl->GetAgent(agentId);
    ag.currentState=0; ag.normalizedTime=0; ag.blendProgress=0;
}
uint32_t AnimationStateMachine::GetCurrentState (uint32_t agentId) const {
    auto it=m_impl->agents.find(agentId); return it!=m_impl->agents.end()?it->second.currentState:0;
}
float AnimationStateMachine::GetNormalizedTime(uint32_t agentId) const {
    auto it=m_impl->agents.find(agentId); return it!=m_impl->agents.end()?it->second.normalizedTime:0;
}

void  AnimationStateMachine::SetBool (uint32_t id,const std::string& n,bool v)   { m_impl->GetAgent(id).bools[n]=v; }
void  AnimationStateMachine::SetFloat(uint32_t id,const std::string& n,float v)  { m_impl->GetAgent(id).floats[n]=v; }
void  AnimationStateMachine::SetInt  (uint32_t id,const std::string& n,int32_t v){ m_impl->GetAgent(id).ints[n]=v; }
bool  AnimationStateMachine::GetBool (uint32_t id,const std::string& n) const {
    auto it=m_impl->agents.find(id); if(it==m_impl->agents.end()) return false;
    auto jt=it->second.bools.find(n); return jt!=it->second.bools.end()&&jt->second;
}
float AnimationStateMachine::GetFloat(uint32_t id,const std::string& n) const {
    auto it=m_impl->agents.find(id); if(it==m_impl->agents.end()) return 0;
    auto jt=it->second.floats.find(n); return jt!=it->second.floats.end()?jt->second:0;
}
int32_t AnimationStateMachine::GetInt(uint32_t id,const std::string& n) const {
    auto it=m_impl->agents.find(id); if(it==m_impl->agents.end()) return 0;
    auto jt=it->second.ints.find(n); return jt!=it->second.ints.end()?jt->second:0;
}

void AnimationStateMachine::AddLayer(uint32_t lid, const std::vector<uint32_t>& bones){
    LayerData ld; ld.id=lid; ld.maskBones=bones;
    m_impl->layers.push_back(ld);
}
void AnimationStateMachine::SetLayerWeight(uint32_t lid, float w){
    for(auto& l:m_impl->layers) if(l.id==lid){ l.weight=w; return; }
}

void AnimationStateMachine::SetOnEnter(uint32_t sid,std::function<void(uint32_t)> cb){
    auto* s=m_impl->FindState(sid); if(s) s->onEnter=cb;
}
void AnimationStateMachine::SetOnExit(uint32_t sid,std::function<void(uint32_t)> cb){
    auto* s=m_impl->FindState(sid); if(s) s->onExit=cb;
}
void AnimationStateMachine::SetOnTransition(uint32_t from,uint32_t to,std::function<void(uint32_t)> cb){
    for(auto& tr:m_impl->transitions) if(tr.fromId==from&&tr.toId==to){ tr.onTransition=cb; return; }
}

} // namespace Engine
