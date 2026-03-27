#include "Runtime/UI/UIAnimator/UIAnimator/UIAnimator.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct Keyframe { float time, value, easeIn, easeOut; };

struct AnimTrack {
    UIAnimProperty prop;
    std::vector<Keyframe> keys;
    float Eval(float t) const {
        if(keys.empty()) return 0;
        if(t<=keys.front().time) return keys.front().value;
        if(t>=keys.back().time) return keys.back().value;
        for(uint32_t i=1;i<keys.size();i++){
            if(t<=keys[i].time){
                float dt=keys[i].time-keys[i-1].time;
                float u=dt>0?(t-keys[i-1].time)/dt:1;
                // Cubic hermite with ease
                float eu=u*u*(3-2*u); // smoothstep by default
                return keys[i-1].value*(1-eu)+keys[i].value*eu;
            }
        }
        return keys.back().value;
    }
};

struct AnimDef {
    std::unordered_map<uint8_t,AnimTrack> tracks;
    bool loop{false}, pingPong{false};
    std::function<void(uint32_t)> onComplete, onLoop;
    float GetDuration() const {
        float d=0;
        for(auto& [k,tr]:tracks) if(!tr.keys.empty()) d=std::max(d,tr.keys.back().time);
        return d;
    }
};

struct PlayState {
    uint32_t animId;
    float    time{0};
    bool     forward{true};
    bool     playing{true};
    bool     paused {false};
    std::unordered_map<uint8_t,float> values;
    std::vector<uint32_t> sequence;
    uint32_t seqIndex{0};
};

struct UIAnimator::Impl {
    std::unordered_map<uint32_t,AnimDef>                        anims;
    std::unordered_map<uint32_t,PlayState>                      playing; // key = targetId

    AnimDef* Find(uint32_t id){ auto it=anims.find(id); return it!=anims.end()?&it->second:nullptr; }
    static uint32_t StateKey(uint32_t anim, uint32_t target){ return (anim<<16)^target; }
};

UIAnimator::UIAnimator(): m_impl(new Impl){}
UIAnimator::~UIAnimator(){ Shutdown(); delete m_impl; }
void UIAnimator::Init(){}
void UIAnimator::Shutdown(){ m_impl->anims.clear(); m_impl->playing.clear(); }
void UIAnimator::Reset(){ m_impl->playing.clear(); }

void UIAnimator::CreateAnimation (uint32_t id){ m_impl->anims[id]=AnimDef{}; }
void UIAnimator::DestroyAnimation(uint32_t id){ m_impl->anims.erase(id); }

bool UIAnimator::AddKeyframe(uint32_t animId, UIAnimProperty prop,
                              float time, float value, float easeIn, float easeOut){
    auto* a=m_impl->Find(animId); if(!a) return false;
    auto& track=a->tracks[(uint8_t)prop];
    track.prop=prop;
    Keyframe kf{time,value,easeIn,easeOut};
    // Insert in sorted order
    auto it=std::lower_bound(track.keys.begin(),track.keys.end(),kf,
                              [](const Keyframe& a, const Keyframe& b){return a.time<b.time;});
    track.keys.insert(it,kf);
    return true;
}

void UIAnimator::SetLoop    (uint32_t id, bool on){ auto* a=m_impl->Find(id); if(a) a->loop=on; }
void UIAnimator::SetPingPong(uint32_t id, bool on){ auto* a=m_impl->Find(id); if(a) a->pingPong=on; }

void UIAnimator::Play(uint32_t animId, uint32_t targetId){
    uint32_t key=Impl::StateKey(animId,targetId);
    PlayState ps; ps.animId=animId;
    m_impl->playing[key]=ps;
}
void UIAnimator::Stop(uint32_t animId, uint32_t targetId){
    m_impl->playing.erase(Impl::StateKey(animId,targetId));
}
void UIAnimator::Pause(uint32_t animId, uint32_t targetId){
    auto it=m_impl->playing.find(Impl::StateKey(animId,targetId));
    if(it!=m_impl->playing.end()) it->second.paused=!it->second.paused;
}
bool UIAnimator::IsPlaying(uint32_t animId, uint32_t targetId) const {
    return m_impl->playing.count(Impl::StateKey(animId,targetId))>0;
}

void UIAnimator::SequencePlay(const std::vector<uint32_t>& ids, uint32_t targetId){
    if(ids.empty()) return;
    uint32_t key=Impl::StateKey(ids[0],targetId);
    PlayState ps; ps.animId=ids[0];
    ps.sequence=ids; ps.seqIndex=0;
    m_impl->playing[key]=ps;
}

void UIAnimator::Tick(float dt){
    std::vector<uint32_t> toRemove;
    for(auto& [key,ps]:m_impl->playing){
        if(ps.paused) continue;
        auto* a=m_impl->Find(ps.animId); if(!a){ toRemove.push_back(key); continue; }
        float dur=a->GetDuration(); if(dur<=0){ toRemove.push_back(key); continue; }
        ps.time+=ps.forward?dt:-dt;
        // Update values
        for(auto& [prop,tr]:a->tracks) ps.values[(uint8_t)prop]=tr.Eval(ps.time);
        // End check
        bool ended=ps.forward?(ps.time>=dur):(ps.time<=0);
        if(ended){
            if(a->loop){
                ps.time=ps.forward?0:dur;
                if(a->pingPong) ps.forward=!ps.forward;
                if(a->onLoop) a->onLoop(key>>16); // approximate targetId
            } else {
                ps.time=ps.forward?dur:0;
                if(a->onComplete) a->onComplete(key>>16);
                // Sequence advance
                if(!ps.sequence.empty()&&ps.seqIndex+1<ps.sequence.size()){
                    ps.seqIndex++; ps.animId=ps.sequence[ps.seqIndex]; ps.time=0;
                } else {
                    toRemove.push_back(key);
                }
            }
        }
    }
    for(auto k:toRemove) m_impl->playing.erase(k);
}

float UIAnimator::GetValue(uint32_t animId, uint32_t targetId, UIAnimProperty prop) const {
    uint32_t key=Impl::StateKey(animId,targetId);
    auto it=m_impl->playing.find(key); if(it==m_impl->playing.end()) return 0;
    auto vit=it->second.values.find((uint8_t)prop);
    return vit!=it->second.values.end()?vit->second:0;
}

void UIAnimator::SetOnComplete(uint32_t id,std::function<void(uint32_t)> cb){ auto* a=m_impl->Find(id); if(a) a->onComplete=cb; }
void UIAnimator::SetOnLoop    (uint32_t id,std::function<void(uint32_t)> cb){ auto* a=m_impl->Find(id); if(a) a->onLoop=cb; }

} // namespace Runtime
