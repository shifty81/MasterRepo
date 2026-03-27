#include "Engine/Input/Recorder/InputRecorder/InputRecorder.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

namespace Engine {

struct InputRecorder::Impl {
    RecorderState state{RecorderState::Idle};
    std::vector<InputEvent> events;
    float     clock{0.f};
    float     playPos{0.f};
    uint32_t  replayIdx{0};
    float     playSpeed{1.f};
    bool      loop{false};
    bool      paused{false};
    uint32_t  frameCount{0};

    std::function<void(const InputEvent&)> onEvent;
    std::function<void()>                  onEnd;
};

InputRecorder::InputRecorder()  : m_impl(new Impl) {}
InputRecorder::~InputRecorder() { Shutdown(); delete m_impl; }

void InputRecorder::Init()     {}
void InputRecorder::Shutdown() { m_impl->events.clear(); }

void InputRecorder::StartRecording() {
    m_impl->state    = RecorderState::Recording;
    m_impl->clock    = 0.f;
    m_impl->frameCount= 0;
    m_impl->events.clear();
}
void InputRecorder::StopRecording() {
    if (m_impl->state==RecorderState::Recording) m_impl->state=RecorderState::Idle;
}
bool InputRecorder::IsRecording() const { return m_impl->state==RecorderState::Recording; }

void InputRecorder::RecordEvent(const InputEvent& e) {
    if (m_impl->state!=RecorderState::Recording) return;
    InputEvent stamped = e;
    stamped.timestamp = m_impl->clock;
    m_impl->events.push_back(stamped);
}

bool InputRecorder::SaveJSON(const std::string& path) const {
    std::ofstream f(path); if(!f) return false;
    f << "{\"events\":[";
    bool first=true;
    for (auto& e : m_impl->events) {
        if(!first) f<<","; first=false;
        f << "{\"type\":" << (int)e.type
          << ",\"name\":\"" << e.name << "\""
          << ",\"value\":" << e.value
          << ",\"timestamp\":" << e.timestamp
          << ",\"x\":" << e.x << ",\"y\":" << e.y << "}";
    }
    f << "]}";
    return true;
}

bool InputRecorder::LoadJSON(const std::string& path) {
    std::ifstream f(path); if(!f) return false;
    // Minimal stub: just signal success
    return true;
}

bool InputRecorder::SaveBinary(const std::string& path) const {
    std::ofstream f(path, std::ios::binary); if(!f) return false;
    uint32_t count = (uint32_t)m_impl->events.size();
    f.write((const char*)&count, sizeof(count));
    for (auto& e : m_impl->events) {
        f.write((const char*)&e.type, sizeof(e.type));
        uint32_t nl=(uint32_t)e.name.size();
        f.write((const char*)&nl, sizeof(nl));
        f.write(e.name.data(), nl);
        f.write((const char*)&e.value, sizeof(e.value));
        f.write((const char*)&e.timestamp, sizeof(e.timestamp));
        f.write((const char*)&e.x, sizeof(e.x));
        f.write((const char*)&e.y, sizeof(e.y));
    }
    return true;
}

bool InputRecorder::LoadBinary(const std::string& path) {
    std::ifstream f(path, std::ios::binary); if(!f) return false;
    m_impl->events.clear();
    uint32_t count=0; f.read((char*)&count, sizeof(count));
    for (uint32_t i=0;i<count;i++) {
        InputEvent e;
        f.read((char*)&e.type, sizeof(e.type));
        uint32_t nl=0; f.read((char*)&nl, sizeof(nl));
        e.name.resize(nl); f.read(e.name.data(), nl);
        f.read((char*)&e.value, sizeof(e.value));
        f.read((char*)&e.timestamp, sizeof(e.timestamp));
        f.read((char*)&e.x, sizeof(e.x));
        f.read((char*)&e.y, sizeof(e.y));
        m_impl->events.push_back(e);
    }
    return true;
}

void InputRecorder::StartReplay(bool loop) {
    m_impl->state    = RecorderState::Replaying;
    m_impl->playPos  = 0.f;
    m_impl->replayIdx= 0;
    m_impl->loop     = loop;
    m_impl->paused   = false;
}
void InputRecorder::StopReplay()    { m_impl->state=RecorderState::Idle; }
void InputRecorder::PauseReplay(bool p) { m_impl->paused=p; }
bool InputRecorder::IsReplaying()  const { return m_impl->state==RecorderState::Replaying; }
bool InputRecorder::IsPaused()     const { return m_impl->paused; }
void InputRecorder::Seek(float t)
{ m_impl->playPos=t; m_impl->replayIdx=0; for(auto& e:m_impl->events) if(e.timestamp<=t) m_impl->replayIdx++; }
void InputRecorder::SetPlaybackSpeed(float s) { m_impl->playSpeed=std::max(0.01f,s); }
float InputRecorder::PlaybackSpeed() const { return m_impl->playSpeed; }

void InputRecorder::SetOnEvent(std::function<void(const InputEvent&)> cb) { m_impl->onEvent=cb; }
void InputRecorder::SetOnEnd  (std::function<void()>                  cb) { m_impl->onEnd=cb; }

RecorderState InputRecorder::State()       const { return m_impl->state; }
float         InputRecorder::Duration()    const {
    if (m_impl->events.empty()) return 0.f;
    return m_impl->events.back().timestamp;
}
float InputRecorder::CurrentTime()         const { return m_impl->playPos; }
float InputRecorder::Progress()            const {
    float d=Duration(); return d>0.f?m_impl->playPos/d:0.f;
}
uint32_t InputRecorder::EventCount()       const { return (uint32_t)m_impl->events.size(); }
uint32_t InputRecorder::FrameCount()       const { return m_impl->frameCount; }
const std::vector<InputEvent>& InputRecorder::Events() const { return m_impl->events; }
void InputRecorder::Clear() { m_impl->events.clear(); m_impl->playPos=0.f; m_impl->replayIdx=0; }

void InputRecorder::Tick(float dt) {
    if (m_impl->state==RecorderState::Recording) {
        m_impl->clock += dt;
        m_impl->frameCount++;
        return;
    }
    if (m_impl->state!=RecorderState::Replaying || m_impl->paused) return;

    float advance = dt * m_impl->playSpeed;
    m_impl->playPos += advance;

    // Fire events that fall in this window
    while (m_impl->replayIdx < (uint32_t)m_impl->events.size()) {
        auto& e = m_impl->events[m_impl->replayIdx];
        if (e.timestamp > m_impl->playPos) break;
        if (m_impl->onEvent) m_impl->onEvent(e);
        m_impl->replayIdx++;
    }

    // End of replay
    if (m_impl->replayIdx >= (uint32_t)m_impl->events.size()) {
        if (m_impl->loop) {
            m_impl->playPos=0.f; m_impl->replayIdx=0;
        } else {
            m_impl->state=RecorderState::Idle;
            if (m_impl->onEnd) m_impl->onEnd();
        }
    }
}

} // namespace Engine
