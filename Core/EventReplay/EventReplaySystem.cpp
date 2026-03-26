#include "Core/EventReplay/EventReplaySystem.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace Core {

static uint64_t NowMs(){
    using namespace std::chrono;
    return static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct EventReplaySystem::Impl {
    ReplayState    state{ReplayState::Idle};
    uint64_t       recordStartMs{0};
    uint64_t       playbackCursorMs{0};
    float          playbackSpeed{1.f};
    size_t         playbackHead{0};  ///< index into events[]

    std::vector<ReplayEvent>    events;
    std::vector<ReplayEvent>    pending;
    std::unordered_set<std::string> disabledCategories;

    std::function<void(const ReplayEvent&)> onFired;
    std::function<void()>                   onPlayEnd;
};

EventReplaySystem::EventReplaySystem() : m_impl(new Impl()) {}
EventReplaySystem::~EventReplaySystem() { delete m_impl; }
void EventReplaySystem::Init()     {}
void EventReplaySystem::Shutdown() { StopRecording(); StopPlayback(); }

void EventReplaySystem::StartRecording() {
    m_impl->state          = ReplayState::Recording;
    m_impl->recordStartMs  = NowMs();
    m_impl->events.clear();
}

void EventReplaySystem::StopRecording() {
    if(m_impl->state==ReplayState::Recording) m_impl->state=ReplayState::Idle;
}

void EventReplaySystem::RecordEvent(const ReplayEvent& event) {
    if(m_impl->state!=ReplayState::Recording) return;
    if(m_impl->disabledCategories.count(event.category)) return;
    ReplayEvent e=event;
    if(!e.timestampMs) e.timestampMs=NowMs()-m_impl->recordStartMs;
    m_impl->events.push_back(e);
}

void EventReplaySystem::RecordEvent(const std::string& cat, const std::string& type,
                                     const std::string& payload) {
    RecordEvent({NowMs()-m_impl->recordStartMs, cat, type, payload});
}

void EventReplaySystem::StartPlayback(float speed) {
    m_impl->state            = ReplayState::PlayingBack;
    m_impl->playbackSpeed    = speed;
    m_impl->playbackCursorMs = 0;
    m_impl->playbackHead     = 0;
    m_impl->pending.clear();
}

void EventReplaySystem::StopPlayback() {
    if(m_impl->state==ReplayState::PlayingBack||m_impl->state==ReplayState::Paused)
        m_impl->state=ReplayState::Idle;
}

void EventReplaySystem::PausePlayback()  { if(m_impl->state==ReplayState::PlayingBack) m_impl->state=ReplayState::Paused; }
void EventReplaySystem::ResumePlayback() { if(m_impl->state==ReplayState::Paused)      m_impl->state=ReplayState::PlayingBack; }
void EventReplaySystem::SetPlaybackSpeed(float s) { m_impl->playbackSpeed=std::max(0.1f,std::min(10.f,s)); }

void EventReplaySystem::SeekToMs(uint64_t ms) {
    m_impl->playbackCursorMs = ms;
    m_impl->playbackHead     = 0;
    while(m_impl->playbackHead<m_impl->events.size() &&
          m_impl->events[m_impl->playbackHead].timestampMs < ms)
        ++m_impl->playbackHead;
}

std::vector<ReplayEvent> EventReplaySystem::Tick(float dt) {
    std::vector<ReplayEvent> fired;
    if(m_impl->state!=ReplayState::PlayingBack) return fired;
    m_impl->playbackCursorMs += static_cast<uint64_t>(dt*1000.f*m_impl->playbackSpeed);
    while(m_impl->playbackHead<m_impl->events.size()){
        auto& ev=m_impl->events[m_impl->playbackHead];
        if(ev.timestampMs>m_impl->playbackCursorMs) break;
        if(!m_impl->disabledCategories.count(ev.category)){
            fired.push_back(ev);
            if(m_impl->onFired) m_impl->onFired(ev);
        }
        ++m_impl->playbackHead;
    }
    if(m_impl->playbackHead>=m_impl->events.size()){
        m_impl->state=ReplayState::Idle;
        if(m_impl->onPlayEnd) m_impl->onPlayEnd();
    }
    return fired;
}

std::vector<ReplayEvent> EventReplaySystem::DrainPending() {
    auto out=std::move(m_impl->pending); m_impl->pending.clear(); return out;
}

ReplayState EventReplaySystem::GetState()           const { return m_impl->state; }
bool        EventReplaySystem::IsRecording()        const { return m_impl->state==ReplayState::Recording; }
bool        EventReplaySystem::IsPlayingBack()      const { return m_impl->state==ReplayState::PlayingBack; }
uint64_t    EventReplaySystem::CurrentTimeMs()      const { return m_impl->playbackCursorMs; }
float       EventReplaySystem::PlaybackProgress()   const {
    uint64_t total=TotalDurationMs(); return total ? (float)m_impl->playbackCursorMs/total : 0.f;
}

uint64_t EventReplaySystem::TotalDurationMs() const {
    return m_impl->events.empty() ? 0 : m_impl->events.back().timestampMs;
}
uint32_t EventReplaySystem::RecordedEventCount() const {
    return static_cast<uint32_t>(m_impl->events.size());
}

void EventReplaySystem::EnableCategory(const std::string& c)  { m_impl->disabledCategories.erase(c); }
void EventReplaySystem::DisableCategory(const std::string& c) { m_impl->disabledCategories.insert(c); }

bool EventReplaySystem::SaveToFile(const std::string& path) const {
    std::ofstream f(path); if(!f.is_open()) return false;
    f<<"{\"events\":[\n";
    for(size_t i=0;i<m_impl->events.size();++i){
        auto& e=m_impl->events[i];
        if(i) f<<",\n";
        f<<"  {\"ts\":"<<e.timestampMs<<",\"cat\":\""<<e.category
         <<"\",\"type\":\""<<e.type<<"\",\"payload\":"<<e.payload<<"}";
    }
    f<<"\n]}\n";
    return true;
}

bool EventReplaySystem::LoadFromFile(const std::string& path) {
    std::ifstream f(path); return f.is_open(); // full impl: parse JSON
}

void EventReplaySystem::ClearRecording() { m_impl->events.clear(); }
void EventReplaySystem::OnEventFired(std::function<void(const ReplayEvent&)> cb) { m_impl->onFired=std::move(cb); }
void EventReplaySystem::OnPlaybackEnd(std::function<void()> cb) { m_impl->onPlayEnd=std::move(cb); }

} // namespace Core
