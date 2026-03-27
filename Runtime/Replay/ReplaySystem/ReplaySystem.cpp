#include "Runtime/Replay/ReplaySystem/ReplaySystem.h"
#include <algorithm>
#include <deque>
#include <fstream>
#include <sstream>
#include <vector>

namespace Runtime {

struct ReplaySystem::Impl {
    std::deque<ReplayFrame>    frames;
    ReplayMode                 mode{ReplayMode::Idle};
    float                      captureTime{0.f};
    float                      playTime{0.f};
    float                      playSpeed{1.f};
    float                      bufferSeconds{30.f};
    uint32_t                   captureRate{20};      ///< fps
    float                      captureInterval{0.f};
    float                      captureAccum{0.f};
    bool                       loop{false};
    bool                       paused{false};
    uint32_t                   playFrameIdx{0};

    std::function<void(const ReplayFrame&)> onApply;
    std::function<void()>                   onEnd;
};

ReplaySystem::ReplaySystem()  : m_impl(new Impl) {}
ReplaySystem::~ReplaySystem() { Shutdown(); delete m_impl; }

void ReplaySystem::Init(float bufferSec, uint32_t rate) {
    m_impl->bufferSeconds    = bufferSec;
    m_impl->captureRate      = rate;
    m_impl->captureInterval  = rate>0 ? 1.f/(float)rate : 1.f/20.f;
}
void ReplaySystem::Shutdown() { m_impl->frames.clear(); }

void ReplaySystem::StartCapture() {
    m_impl->frames.clear();
    m_impl->captureTime  = 0.f;
    m_impl->captureAccum = 0.f;
    m_impl->mode         = ReplayMode::Capturing;
}
void ReplaySystem::StopCapture() {
    if(m_impl->mode==ReplayMode::Capturing) m_impl->mode=ReplayMode::Idle;
}
bool ReplaySystem::IsCapturing() const { return m_impl->mode==ReplayMode::Capturing; }

void ReplaySystem::AppendFrame(const std::vector<EntityState>& states)
{
    if(m_impl->mode!=ReplayMode::Capturing) return;
    ReplayFrame f; f.timestamp=m_impl->captureTime; f.entities=states;
    m_impl->frames.push_back(f);
    // Evict old frames beyond buffer
    while(!m_impl->frames.empty() &&
          m_impl->captureTime - m_impl->frames.front().timestamp > m_impl->bufferSeconds)
        m_impl->frames.pop_front();
}

void ReplaySystem::StartPlayback(bool loop) {
    m_impl->mode=ReplayMode::Playing;
    m_impl->playTime=0.f; m_impl->playFrameIdx=0;
    m_impl->loop=loop; m_impl->paused=false;
}
void ReplaySystem::StopPlayback() { m_impl->mode=ReplayMode::Idle; }
void ReplaySystem::Pause(bool p)  { m_impl->paused=p; if(!p) m_impl->mode=ReplayMode::Playing; }
bool ReplaySystem::IsPlaying() const { return m_impl->mode==ReplayMode::Playing; }
bool ReplaySystem::IsPaused()  const { return m_impl->paused; }

void ReplaySystem::Seek(float t) {
    m_impl->playTime=t; m_impl->playFrameIdx=0;
    for(uint32_t i=0;i<(uint32_t)m_impl->frames.size();i++)
        if(m_impl->frames[i].timestamp<=t) m_impl->playFrameIdx=i;
}

void ReplaySystem::SetPlaybackSpeed(float s) { m_impl->playSpeed=std::max(0.01f,s); }
float ReplaySystem::PlaybackSpeed()  const   { return m_impl->playSpeed; }

void ReplaySystem::SetOnApplyFrame(std::function<void(const ReplayFrame&)> cb) { m_impl->onApply=cb; }
void ReplaySystem::SetOnEnd       (std::function<void()>                   cb) { m_impl->onEnd=cb; }

ReplayMode ReplaySystem::Mode()       const { return m_impl->mode; }
float      ReplaySystem::Duration()   const {
    if(m_impl->frames.empty()) return 0.f;
    return m_impl->frames.back().timestamp - m_impl->frames.front().timestamp;
}
float      ReplaySystem::CurrentTime()const { return m_impl->playTime; }
float      ReplaySystem::Progress()   const {
    float d=Duration(); return d>0.f?m_impl->playTime/d:0.f;
}
uint32_t   ReplaySystem::FrameCount() const { return (uint32_t)m_impl->frames.size(); }
size_t     ReplaySystem::MemoryBytes()const {
    size_t sz=0;
    for(auto& f:m_impl->frames) {
        sz+=sizeof(f.timestamp);
        for(auto& e:f.entities) sz+=sizeof(e.entityId)+e.data.size();
    }
    return sz;
}

void ReplaySystem::Clear() { m_impl->frames.clear(); m_impl->playTime=0.f; m_impl->playFrameIdx=0; }

bool ReplaySystem::SaveJSON(const std::string& path) const {
    std::ofstream f(path); if(!f) return false;
    f << "{\"frames\":" << m_impl->frames.size() << ",\"duration\":" << Duration() << "}";
    return true;
}
bool ReplaySystem::LoadJSON(const std::string& path) {
    std::ifstream f(path); return f.good();
}

void ReplaySystem::Tick(float dt)
{
    if(m_impl->mode==ReplayMode::Capturing) {
        m_impl->captureTime  += dt;
        m_impl->captureAccum += dt;
        // AppendFrame is called externally at caller's tick rate;
        // captureAccum used to gate if caller calls Tick more often
    }
    else if(m_impl->mode==ReplayMode::Playing && !m_impl->paused) {
        m_impl->playTime += dt * m_impl->playSpeed;
        if(!m_impl->frames.empty()) {
            float offset = m_impl->frames.front().timestamp;
            float absTime= offset + m_impl->playTime;
            // Find frame
            while(m_impl->playFrameIdx+1 < (uint32_t)m_impl->frames.size() &&
                  m_impl->frames[m_impl->playFrameIdx+1].timestamp <= absTime)
                m_impl->playFrameIdx++;
            if(m_impl->onApply)
                m_impl->onApply(m_impl->frames[m_impl->playFrameIdx]);
            // End check
            if(m_impl->playTime >= Duration()) {
                if(m_impl->loop) { m_impl->playTime=0.f; m_impl->playFrameIdx=0; }
                else { m_impl->mode=ReplayMode::Idle; if(m_impl->onEnd) m_impl->onEnd(); }
            }
        }
    }
}

} // namespace Runtime
