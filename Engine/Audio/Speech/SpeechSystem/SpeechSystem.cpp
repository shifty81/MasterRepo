#include "Engine/Audio/Speech/SpeechSystem/SpeechSystem.h"
#include <algorithm>
#include <sstream>
#include <vector>

namespace Engine {

// Minimal viseme table: character → viseme index
static uint8_t CharToViseme(char c){
    c=(char)tolower((unsigned char)c);
    switch(c){
        case 'a': case 'e': case 'i': case 'o': case 'u': return 1;
        case 'p': case 'b': case 'm': return 2;
        case 'f': case 'v': return 3;
        case 't': case 'd': case 'n': return 4;
        case 'k': case 'g': return 5;
        case 's': case 'z': return 6;
        case 'l': case 'r': return 7;
        case ' ': return 0;
        default:  return 8;
    }
}

struct VoiceProfile {
    uint32_t    id;
    std::string name;
    float       pitch{0};
    float       rate {1};
};

struct SpeechLine {
    uint32_t    lineId;
    std::string text;
    float       totalTime;
    float       elapsed;
    uint8_t     currentViseme;
};

struct VoiceState {
    VoiceProfile profile;
    std::vector<SpeechLine> queue;
    std::function<void(uint8_t,float)>              onViseme;
    std::function<void(uint32_t,const std::string&)> onLineStart;
    std::function<void(uint32_t)>                   onLineEnd;
};

struct SpeechSystem::Impl {
    std::unordered_map<uint32_t,VoiceState> voices;
    std::unordered_map<char,std::string>    phonemeMap;
    uint32_t nextLineId{1};

    VoiceState* Find(uint32_t id){
        auto it=voices.find(id); return it!=voices.end()?&it->second:nullptr;
    }
};

SpeechSystem::SpeechSystem(): m_impl(new Impl){}
SpeechSystem::~SpeechSystem(){ Shutdown(); delete m_impl; }
void SpeechSystem::Init(){}
void SpeechSystem::Shutdown(){ m_impl->voices.clear(); }
void SpeechSystem::Reset(){ m_impl->voices.clear(); m_impl->nextLineId=1; }

void SpeechSystem::RegisterVoice(uint32_t id, const std::string& name){
    VoiceState vs; vs.profile.id=id; vs.profile.name=name;
    m_impl->voices[id]=vs;
}
void SpeechSystem::SetVoicePitch(uint32_t id, float s){
    auto* v=m_impl->Find(id); if(v) v->profile.pitch=s;
}
void SpeechSystem::SetVoiceRate(uint32_t id, float r){
    auto* v=m_impl->Find(id); if(v) v->profile.rate=std::max(0.1f,r);
}

uint32_t SpeechSystem::Speak(uint32_t voiceId, const std::string& text){
    auto* vs=m_impl->Find(voiceId); if(!vs) return 0;
    SpeechLine line;
    line.lineId=m_impl->nextLineId++;
    line.text=text;
    float rate=vs->profile.rate;
    line.totalTime=(float)text.size()/(rate*10.f); // ~10 chars/s
    line.elapsed=0; line.currentViseme=0;
    vs->queue.push_back(line);
    return line.lineId;
}
void SpeechSystem::Stop(uint32_t voiceId){
    auto* vs=m_impl->Find(voiceId); if(vs) vs->queue.clear();
}
void SpeechSystem::StopAll(){
    for(auto& [id,vs]:m_impl->voices) vs.queue.clear();
}

void SpeechSystem::Tick(float dt){
    for(auto& [id,vs]:m_impl->voices){
        if(vs.queue.empty()) continue;
        SpeechLine& line=vs.queue.front();
        bool first=(line.elapsed==0);
        if(first&&vs.onLineStart) vs.onLineStart(line.lineId,line.text);
        line.elapsed+=dt*vs.profile.rate;
        // Current viseme
        float rate=vs.profile.rate;
        size_t charIdx=(size_t)(line.elapsed*rate*10.f);
        if(charIdx<line.text.size())
            line.currentViseme=CharToViseme(line.text[charIdx]);
        else line.currentViseme=0;
        if(vs.onViseme) vs.onViseme(line.currentViseme, 1.f);
        if(line.elapsed>=line.totalTime){
            uint32_t lid=line.lineId;
            vs.queue.erase(vs.queue.begin());
            if(vs.onLineEnd) vs.onLineEnd(lid);
        }
    }
}

uint8_t SpeechSystem::GetCurrentViseme(uint32_t id) const {
    auto it=m_impl->voices.find(id);
    if(it==m_impl->voices.end()||it->second.queue.empty()) return 0;
    return it->second.queue.front().currentViseme;
}
float SpeechSystem::GetPlaybackProgress(uint32_t id) const {
    auto it=m_impl->voices.find(id);
    if(it==m_impl->voices.end()||it->second.queue.empty()) return 0;
    auto& l=it->second.queue.front();
    return l.totalTime>0?std::min(1.f,l.elapsed/l.totalTime):1.f;
}
bool SpeechSystem::IsPlaying(uint32_t id) const {
    auto it=m_impl->voices.find(id);
    return it!=m_impl->voices.end()&&!it->second.queue.empty();
}

void SpeechSystem::SetOnViseme(uint32_t id,
    std::function<void(uint8_t,float)> cb){
    auto* v=m_impl->Find(id); if(v) v->onViseme=cb;
}
void SpeechSystem::SetOnLineStart(uint32_t id,
    std::function<void(uint32_t,const std::string&)> cb){
    auto* v=m_impl->Find(id); if(v) v->onLineStart=cb;
}
void SpeechSystem::SetOnLineEnd(uint32_t id,
    std::function<void(uint32_t)> cb){
    auto* v=m_impl->Find(id); if(v) v->onLineEnd=cb;
}

void SpeechSystem::SetPhonemeMap(const std::unordered_map<char,std::string>& m){
    m_impl->phonemeMap=m;
}
std::string SpeechSystem::Preprocess(const std::string& text) const {
    // Expand digits → words (simple stub)
    std::string out;
    for(char c:text){
        if(c>='0'&&c<='9'){
            static const char* words[]={"zero","one","two","three","four",
                                        "five","six","seven","eight","nine"};
            out+=words[c-'0']; out+=' ';
        } else { out+=c; }
    }
    return out;
}

} // namespace Engine
