#include "Runtime/UI/Subtitle/SubtitleSystem/SubtitleSystem.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct SubtitleSystem::Impl {
    std::vector<SubtitleEntry> entries;
    uint32_t nextId{1};
    uint32_t maxEntries{100};
    float    time{0.f};
    bool     paused{false};
    uint32_t maxLineWidth{50};

    std::unordered_map<std::string,std::vector<float>> speakerColours;
    std::function<std::string(const std::string&)>      translateFn;
    std::function<void(const SubtitleEntry&)>            onShow;
    std::function<void(uint32_t)>                        onHide;

    SubtitleEntry* Find(uint32_t id) {
        for(auto& e:entries) if(e.id==id) return &e; return nullptr;
    }
    const SubtitleEntry* Find(uint32_t id) const {
        for(auto& e:entries) if(e.id==id) return &e; return nullptr;
    }

    std::vector<std::string> WrapText(const std::string& text, uint32_t width) {
        std::vector<std::string> lines;
        std::istringstream ss(text);
        std::string word, line;
        while (ss >> word) {
            if (!line.empty() && line.size()+1+word.size() > width) {
                lines.push_back(line); line=word;
            } else {
                if (!line.empty()) line+=" ";
                line+=word;
            }
        }
        if (!line.empty()) lines.push_back(line);
        return lines;
    }
};

SubtitleSystem::SubtitleSystem()  : m_impl(new Impl) {}
SubtitleSystem::~SubtitleSystem() { Shutdown(); delete m_impl; }

void SubtitleSystem::Init(uint32_t max) { m_impl->maxEntries=max; }
void SubtitleSystem::Shutdown() { m_impl->entries.clear(); }

void SubtitleSystem::SetTime(float t) { m_impl->time=t; }
void SubtitleSystem::Pause(bool p)    { m_impl->paused=p; }
void SubtitleSystem::Clear()          { m_impl->entries.clear(); }

uint32_t SubtitleSystem::AddEntry(const SubtitleEntry& e_)
{
    SubtitleEntry e = e_;
    e.id = m_impl->nextId++;
    // Translate
    if (m_impl->translateFn) e.text = m_impl->translateFn(e.text);
    // Wrap
    e.wrappedLines = m_impl->WrapText(e.text, m_impl->maxLineWidth);
    // Speaker colour override
    auto it = m_impl->speakerColours.find(e.speaker);
    if (it!=m_impl->speakerColours.end() && it->second.size()>=4)
        for(int i=0;i<4;i++) e.style.colour[i]=it->second[i];
    m_impl->entries.push_back(e);
    return e.id;
}

void SubtitleSystem::RemoveEntry(uint32_t id) {
    auto& v=m_impl->entries;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& e){return e.id==id;}),v.end());
}
bool SubtitleSystem::HasEntry(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void SubtitleSystem::SetMaxLineWidth(uint32_t w) { m_impl->maxLineWidth=w; }
void SubtitleSystem::SetSpeakerColour(const std::string& spk, const float rgba[4]) {
    m_impl->speakerColours[spk]={rgba[0],rgba[1],rgba[2],rgba[3]};
}
void SubtitleSystem::SetTranslateFn(std::function<std::string(const std::string&)> fn) { m_impl->translateFn=fn; }
void SubtitleSystem::SetOnShow(std::function<void(const SubtitleEntry&)> cb) { m_impl->onShow=cb; }
void SubtitleSystem::SetOnHide(std::function<void(uint32_t)>            cb) { m_impl->onHide=cb; }

std::vector<const SubtitleEntry*> SubtitleSystem::GetActive() const {
    std::vector<const SubtitleEntry*> out;
    for(auto& e:m_impl->entries)
        if(e.active) out.push_back(&e);
    return out;
}
float SubtitleSystem::CurrentTime() const { return m_impl->time; }
bool  SubtitleSystem::IsPaused()    const { return m_impl->paused; }

bool SubtitleSystem::LoadSRT(const std::string& path)
{
    std::ifstream f(path); if(!f) return false;
    // Minimal SRT parser: index, timecode, text, blank line
    std::string line;
    while(std::getline(f,line)) {
        // Skip index number
        if (line.empty()) continue;
        std::string tc;
        if(!std::getline(f,tc)) break;
        // Parse start/end HH:MM:SS,mmm
        auto parseTC = [](const std::string& s) -> float {
            int h=0,m=0,sec=0,ms=0;
            sscanf(s.c_str(), "%d:%d:%d,%d",&h,&m,&sec,&ms);
            return h*3600.f+m*60.f+sec+ms/1000.f;
        };
        auto arrow = tc.find(" --> ");
        if(arrow==std::string::npos) continue;
        float start=parseTC(tc.substr(0,arrow));
        float end  =parseTC(tc.substr(arrow+5));
        std::string text;
        std::string tl;
        while(std::getline(f,tl)&&!tl.empty()) {
            if(!text.empty()) text+=" ";
            text+=tl;
        }
        SubtitleEntry e; e.text=text; e.startTime=start; e.duration=end-start;
        AddEntry(e);
    }
    return true;
}

void SubtitleSystem::Tick(float dt)
{
    if(m_impl->paused) return;
    m_impl->time += dt;
    for(auto& e : m_impl->entries) {
        float end = e.startTime + e.duration;
        bool shouldShow = (m_impl->time >= e.startTime && m_impl->time < end);
        if(shouldShow && !e.active) {
            e.active = true;
            if(m_impl->onShow) m_impl->onShow(e);
        } else if(!shouldShow && e.active) {
            e.active = false;
            if(m_impl->onHide) m_impl->onHide(e.id);
        }
        // Alpha
        if(e.active) {
            float age = m_impl->time - e.startTime;
            float rem = end - m_impl->time;
            if(age < e.fadeIn) e.alpha=age/e.fadeIn;
            else if(rem < e.fadeOut) e.alpha=rem/e.fadeOut;
            else e.alpha=1.f;
        } else { e.alpha=0.f; }
    }
}

} // namespace Runtime
