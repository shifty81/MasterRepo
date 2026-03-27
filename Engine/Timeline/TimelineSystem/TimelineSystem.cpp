#include "Engine/Timeline/TimelineSystem/TimelineSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct Keyframe { float time; float value; };
struct TrackData {
    uint32_t    id;
    std::string name;
    std::vector<Keyframe> keys;
    std::unordered_map<uint32_t,std::function<void()>> keyCallbacks; // time*1000 → cb
};
struct EventMarker { float time; std::string tag; bool fired; };
struct TimelineData {
    uint32_t    id;
    bool        playing{false};
    bool        looping{false};
    float       head{0};
    std::unordered_map<uint32_t,TrackData> tracks;
    std::vector<EventMarker> events;
    std::function<void(float,const std::string&)> onEvent;
    std::function<void()>                         onEnd;
};

struct TimelineSystem::Impl {
    std::unordered_map<uint32_t,TimelineData> timelines;
    TimelineData* Find(uint32_t id){
        auto it=timelines.find(id); return it!=timelines.end()?&it->second:nullptr;
    }
};

TimelineSystem::TimelineSystem(): m_impl(new Impl){}
TimelineSystem::~TimelineSystem(){ Shutdown(); delete m_impl; }
void TimelineSystem::Init(){}
void TimelineSystem::Shutdown(){ m_impl->timelines.clear(); }
void TimelineSystem::Reset(){ m_impl->timelines.clear(); }

void TimelineSystem::CreateTimeline (uint32_t id){ m_impl->timelines[id].id=id; }
void TimelineSystem::DestroyTimeline(uint32_t id){ m_impl->timelines.erase(id); }

void TimelineSystem::AddTrack(uint32_t tlId, uint32_t trackId, const std::string& name){
    auto* tl=m_impl->Find(tlId); if(!tl) return;
    TrackData td; td.id=trackId; td.name=name;
    tl->tracks[trackId]=td;
}
void TimelineSystem::AddKeyframe(uint32_t tlId, uint32_t trackId, float time, float value){
    auto* tl=m_impl->Find(tlId); if(!tl) return;
    auto it=tl->tracks.find(trackId); if(it==tl->tracks.end()) return;
    Keyframe kf; kf.time=time; kf.value=value;
    it->second.keys.push_back(kf);
    std::sort(it->second.keys.begin(),it->second.keys.end(),
              [](const Keyframe& a,const Keyframe& b){return a.time<b.time;});
}
void TimelineSystem::RemoveKeyframe(uint32_t tlId, uint32_t trackId, float time){
    auto* tl=m_impl->Find(tlId); if(!tl) return;
    auto it=tl->tracks.find(trackId); if(it==tl->tracks.end()) return;
    auto& keys=it->second.keys;
    keys.erase(std::remove_if(keys.begin(),keys.end(),
        [time](const Keyframe& k){return std::abs(k.time-time)<1e-4f;}),keys.end());
}

void TimelineSystem::Play(uint32_t id){ auto* tl=m_impl->Find(id); if(tl) tl->playing=true; }
void TimelineSystem::Pause(uint32_t id){ auto* tl=m_impl->Find(id); if(tl) tl->playing=false; }
void TimelineSystem::Stop(uint32_t id){ auto* tl=m_impl->Find(id); if(!tl) return; tl->playing=false; tl->head=0; }
void TimelineSystem::Seek(uint32_t id, float t){ auto* tl=m_impl->Find(id); if(tl) tl->head=t; }
void TimelineSystem::SetLooping(uint32_t id, bool on){ auto* tl=m_impl->Find(id); if(tl) tl->looping=on; }

void TimelineSystem::Tick(float dt){
    for(auto& [id,tl]:m_impl->timelines){
        if(!tl.playing) continue;
        float prev=tl.head;
        tl.head+=dt;
        float dur=GetDuration(id);
        // Fire events
        for(auto& ev:tl.events){
            if(!ev.fired&&tl.head>=ev.time&&prev<ev.time){
                ev.fired=true;
                if(tl.onEvent) tl.onEvent(ev.time,ev.tag);
            }
        }
        // Fire keyframe callbacks
        for(auto& [tid,track]:tl.tracks){
            for(auto& kf:track.keys){
                uint32_t key=(uint32_t)(kf.time*1000);
                auto cit=track.keyCallbacks.find(key);
                if(cit!=track.keyCallbacks.end()&&tl.head>=kf.time&&prev<kf.time)
                    cit->second();
            }
        }
        if(dur>0&&tl.head>=dur){
            if(tl.looping){
                tl.head=std::fmod(tl.head,dur);
                for(auto& ev:tl.events) ev.fired=false;
            } else {
                tl.head=dur; tl.playing=false;
                if(tl.onEnd) tl.onEnd();
            }
        }
    }
}

float TimelineSystem::GetTime(uint32_t id) const {
    auto it=m_impl->timelines.find(id); return it!=m_impl->timelines.end()?it->second.head:0;
}
float TimelineSystem::GetDuration(uint32_t id) const {
    auto it=m_impl->timelines.find(id); if(it==m_impl->timelines.end()) return 0;
    float maxT=0;
    for(auto& [tid,track]:it->second.tracks)
        if(!track.keys.empty()) maxT=std::max(maxT,track.keys.back().time);
    return maxT;
}
float TimelineSystem::Sample(uint32_t tlId, uint32_t trackId, float time) const {
    auto tlit=m_impl->timelines.find(tlId); if(tlit==m_impl->timelines.end()) return 0;
    auto trit=tlit->second.tracks.find(trackId); if(trit==tlit->second.tracks.end()) return 0;
    auto& keys=trit->second.keys;
    if(keys.empty()) return 0;
    if(time<=keys.front().time) return keys.front().value;
    if(time>=keys.back().time)  return keys.back().value;
    // Linear interp
    for(uint32_t i=0;i+1<keys.size();i++){
        if(time>=keys[i].time&&time<=keys[i+1].time){
            float t=(time-keys[i].time)/(keys[i+1].time-keys[i].time);
            return keys[i].value+(keys[i+1].value-keys[i].value)*t;
        }
    }
    return 0;
}
bool TimelineSystem::IsPlaying(uint32_t id) const {
    auto it=m_impl->timelines.find(id); return it!=m_impl->timelines.end()&&it->second.playing;
}
void TimelineSystem::AddEvent(uint32_t tlId, float time, const std::string& tag){
    auto* tl=m_impl->Find(tlId); if(!tl) return;
    EventMarker em; em.time=time; em.tag=tag; em.fired=false;
    tl->events.push_back(em);
}
void TimelineSystem::SetOnEvent(uint32_t tlId,
    std::function<void(float,const std::string&)> cb){
    auto* tl=m_impl->Find(tlId); if(tl) tl->onEvent=cb;
}
void TimelineSystem::SetOnKeyframe(uint32_t tlId,uint32_t trackId,float time,
    std::function<void()> cb){
    auto* tl=m_impl->Find(tlId); if(!tl) return;
    auto it=tl->tracks.find(trackId); if(it==tl->tracks.end()) return;
    it->second.keyCallbacks[(uint32_t)(time*1000)]=cb;
}
void TimelineSystem::SetOnEnd(uint32_t tlId,std::function<void()> cb){
    auto* tl=m_impl->Find(tlId); if(tl) tl->onEnd=cb;
}

} // namespace Engine
