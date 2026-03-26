#include "Runtime/Audio/VoiceManager/VoiceManager.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct ChannelState {
    VoiceLine line;
    float     elapsed{0.f};
    float     duration{3.f};  ///< estimated if audio backend unavailable
    bool      playing{false};
};

struct VoiceManager::Impl {
    std::unordered_map<std::string,ChannelState> channels;  ///< speakerId → state
    std::unordered_map<std::string,float>         cooldowns; ///< "speakerId:audioPath" → remaining
    std::unordered_map<std::string,float>         cooldownOverrides;

    std::function<void(const VoiceLine&)>                 onStart;
    std::function<void(const VoiceLine&)>                 onEnd;
    std::function<void(const std::string&,const std::string&)> onSubtitle;

    bool ShouldInterrupt(const ChannelState& cur, const VoiceLine& newLine) const {
        return !cur.playing || (uint8_t)newLine.priority > (uint8_t)cur.line.priority
               || cur.line.interruptible;
    }

    float CooldownKey(const std::string& spk, const std::string& path){
        auto it=cooldownOverrides.find(path);
        return it!=cooldownOverrides.end()?it->second:5.f; // default 5s
    }
};

VoiceManager::VoiceManager() : m_impl(new Impl()) {}
VoiceManager::~VoiceManager() { delete m_impl; }
void VoiceManager::Init()     {}
void VoiceManager::Shutdown() { StopAll(); }

void VoiceManager::PlayLine(const VoiceLine& line){
    if(IsOnCooldown(line.speakerId, line.audioPath)) return;
    auto& ch=m_impl->channels[line.speakerId];
    if(ch.playing && !m_impl->ShouldInterrupt(ch,line)) return;
    // Stop current
    if(ch.playing && m_impl->onEnd) m_impl->onEnd(ch.line);
    ch.line=line; ch.elapsed=0.f; ch.playing=true;
    // Estimate duration (3 words/second heuristic)
    float wordCount=1.f;
    for(char c:line.text) if(c==' ') ++wordCount;
    ch.duration=std::max(1.f,wordCount/3.f);
    // Set cooldown
    std::string ck=line.speakerId+":"+line.audioPath;
    m_impl->cooldowns[ck]=m_impl->CooldownKey(line.speakerId,line.audioPath);
    if(m_impl->onStart) m_impl->onStart(line);
    if(m_impl->onSubtitle) m_impl->onSubtitle(line.speakerId,line.text);
}

void VoiceManager::StopSpeaker(const std::string& id){
    auto it=m_impl->channels.find(id);
    if(it!=m_impl->channels.end()&&it->second.playing){
        if(m_impl->onEnd) m_impl->onEnd(it->second.line);
        it->second.playing=false;
    }
}
void VoiceManager::StopAll(){
    for(auto&[id,ch]:m_impl->channels) if(ch.playing){ if(m_impl->onEnd) m_impl->onEnd(ch.line); ch.playing=false; }
}

bool VoiceManager::IsSpeakerBusy(const std::string& id) const{
    auto it=m_impl->channels.find(id); return it!=m_impl->channels.end()&&it->second.playing;
}
VoiceLineState VoiceManager::GetSpeakerState(const std::string& id) const{
    auto it=m_impl->channels.find(id); if(it==m_impl->channels.end()) return {};
    VoiceLineState s; s.line=it->second.line; s.elapsed=it->second.elapsed;
    s.duration=it->second.duration; s.playing=it->second.playing;
    return s;
}

std::string VoiceManager::GetCurrentSubtitle(const std::string& id) const{
    auto it=m_impl->channels.find(id); if(it==m_impl->channels.end()||!it->second.playing) return "";
    return it->second.line.text;
}
std::vector<std::pair<std::string,std::string>> VoiceManager::AllActiveSubtitles() const{
    std::vector<std::pair<std::string,std::string>> out;
    for(auto&[id,ch]:m_impl->channels) if(ch.playing) out.push_back({id,ch.line.text});
    return out;
}

bool VoiceManager::IsOnCooldown(const std::string& spk, const std::string& path) const{
    std::string ck=spk+":"+path;
    auto it=m_impl->cooldowns.find(ck); return it!=m_impl->cooldowns.end()&&it->second>0.f;
}
void VoiceManager::SetCooldownOverride(const std::string& path, float cd){ m_impl->cooldownOverrides[path]=cd; }
void VoiceManager::SetSpeakerPosition(const std::string& id, const float pos[3]){
    auto it=m_impl->channels.find(id);
    if(it!=m_impl->channels.end()) for(int i=0;i<3;++i) it->second.line.speakerPos[i]=pos[i];
}

void VoiceManager::Update(float dt){
    // Age channels
    for(auto&[id,ch]:m_impl->channels){
        if(!ch.playing) continue;
        ch.elapsed+=dt;
        if(ch.elapsed>=ch.duration){
            ch.playing=false;
            if(m_impl->onEnd) m_impl->onEnd(ch.line);
            if(m_impl->onSubtitle) m_impl->onSubtitle(id,"");
        }
    }
    // Decay cooldowns
    for(auto& [k,v]:m_impl->cooldowns) v=std::max(0.f,v-dt);
}

void VoiceManager::OnLineStarted(std::function<void(const VoiceLine&)> cb)   { m_impl->onStart=std::move(cb); }
void VoiceManager::OnLineFinished(std::function<void(const VoiceLine&)> cb)  { m_impl->onEnd=std::move(cb); }
void VoiceManager::OnSubtitleChanged(std::function<void(const std::string&,const std::string&)> cb){ m_impl->onSubtitle=std::move(cb); }

} // namespace Runtime
