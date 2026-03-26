#include "Editor/Panels/TimelineEditor/TimelineEditorPanel.h"
#include "Engine/Cinematic/CinematicSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Editor {

struct TimelineEditorPanel::Impl {
    Engine::CinematicSystem*        cinematic{nullptr};
    TimelineEditorState             state;
    std::vector<TimelineTrack>      tracks;
    uint32_t                        nextTrackId{1};

    std::function<void(float)>              onPlayheadMoved;
    std::function<void(uint32_t, float)>    onKeyframeSelected;

    int  dragTrackId{-1};
    float dragKfTime{-1.f};
    bool  draggingPlayhead{false};
};

TimelineEditorPanel::TimelineEditorPanel() : m_impl(new Impl()) {}
TimelineEditorPanel::~TimelineEditorPanel() { delete m_impl; }

void TimelineEditorPanel::Init(Engine::CinematicSystem* cinematic) {
    m_impl->cinematic = cinematic;
}

void TimelineEditorPanel::Shutdown() { Clear(); }

void TimelineEditorPanel::LoadSequence(uint32_t seqId) {
    m_impl->state.activeSeqId = seqId;
    if (!m_impl->cinematic) return;
    Engine::CinemaSequence seq = m_impl->cinematic->GetSequence(seqId);
    m_impl->state.duration = seq.duration > 0.f ? seq.duration : 10.f;
    // Auto-create a Camera track from the sequence keyframes
    if (!seq.keyframes.empty()) {
        uint32_t tid = AddTrack("Camera", "camera.position");
        for (auto& kf : seq.keyframes)
            AddKeyframe(tid, kf.time);
    }
}

void TimelineEditorPanel::Clear() {
    m_impl->tracks.clear();
    m_impl->state = TimelineEditorState{};
}

uint32_t TimelineEditorPanel::AddTrack(const std::string& name,
                                       const std::string& prop) {
    TimelineTrack t;
    t.id           = m_impl->nextTrackId++;
    t.name         = name;
    t.propertyPath = prop;
    m_impl->tracks.push_back(t);
    return t.id;
}

void TimelineEditorPanel::RemoveTrack(uint32_t id) {
    auto& v = m_impl->tracks;
    v.erase(std::remove_if(v.begin(), v.end(),
        [id](const TimelineTrack& t){ return t.id == id; }), v.end());
}

void TimelineEditorPanel::AddKeyframe(uint32_t trackId, float time) {
    for (auto& t : m_impl->tracks) {
        if (t.id == trackId) {
            t.keyframeTimes.push_back(time);
            std::sort(t.keyframeTimes.begin(), t.keyframeTimes.end());
            return;
        }
    }
}

void TimelineEditorPanel::RemoveKeyframe(uint32_t trackId, float time) {
    for (auto& t : m_impl->tracks) {
        if (t.id == trackId) {
            auto& v = t.keyframeTimes;
            v.erase(std::remove_if(v.begin(), v.end(),
                [time](float f){ return std::abs(f - time) < 0.001f; }), v.end());
            return;
        }
    }
}

void TimelineEditorPanel::MoveKeyframe(uint32_t trackId, float oldTime, float newTime) {
    RemoveKeyframe(trackId, oldTime);
    AddKeyframe(trackId, newTime);
}

TimelineTrack TimelineEditorPanel::GetTrack(uint32_t id) const {
    for (auto& t : m_impl->tracks) if (t.id == id) return t;
    return {};
}

std::vector<TimelineTrack> TimelineEditorPanel::AllTracks() const {
    return m_impl->tracks;
}

void TimelineEditorPanel::Play() {
    m_impl->state.playing = true;
    if (m_impl->cinematic && m_impl->state.activeSeqId)
        m_impl->cinematic->Play(m_impl->state.activeSeqId);
}

void TimelineEditorPanel::Pause() {
    m_impl->state.playing = false;
    if (m_impl->cinematic && m_impl->state.activeSeqId)
        m_impl->cinematic->Pause(m_impl->state.activeSeqId);
}

void TimelineEditorPanel::Stop() {
    m_impl->state.playing  = false;
    m_impl->state.playhead = 0.f;
    if (m_impl->cinematic && m_impl->state.activeSeqId)
        m_impl->cinematic->Stop(m_impl->state.activeSeqId);
}

void TimelineEditorPanel::SetLooping(bool loop) {
    m_impl->state.looping = loop;
    if (m_impl->cinematic && m_impl->state.activeSeqId)
        m_impl->cinematic->SetLoop(m_impl->state.activeSeqId, loop);
}

void TimelineEditorPanel::SetPlayhead(float t) {
    m_impl->state.playhead = t;
    if (m_impl->cinematic && m_impl->state.activeSeqId)
        m_impl->cinematic->Seek(m_impl->state.activeSeqId, t);
    if (m_impl->onPlayheadMoved) m_impl->onPlayheadMoved(t);
}

float TimelineEditorPanel::GetPlayhead() const { return m_impl->state.playhead; }

void TimelineEditorPanel::Draw(int /*x*/, int /*y*/, int /*w*/, int /*h*/) {
    // Atlas custom UI rendering hook — actual drawing done by the UI layer
}

void TimelineEditorPanel::Tick(float dt) {
    if (!m_impl->state.playing) return;
    float oldHead = m_impl->state.playhead;
    m_impl->state.playhead += dt;
    if (m_impl->state.playhead > m_impl->state.duration) {
        if (m_impl->state.looping)
            m_impl->state.playhead = std::fmod(m_impl->state.playhead,
                                               m_impl->state.duration);
        else {
            m_impl->state.playhead = m_impl->state.duration;
            m_impl->state.playing  = false;
        }
    }
    if (m_impl->state.playhead != oldHead && m_impl->onPlayheadMoved)
        m_impl->onPlayheadMoved(m_impl->state.playhead);
}

void TimelineEditorPanel::OnMouseMove(int /*x*/, int /*y*/) {}
void TimelineEditorPanel::OnMouseButton(int /*btn*/, bool /*pressed*/,
                                        int /*x*/, int /*y*/) {}
void TimelineEditorPanel::OnMouseScroll(float /*dx*/, float dy) {
    m_impl->state.zoomLevel = std::max(10.f, m_impl->state.zoomLevel + dy * 5.f);
}
void TimelineEditorPanel::OnKey(int /*key*/, bool /*pressed*/) {}

TimelineEditorState TimelineEditorPanel::GetState() const { return m_impl->state; }
void TimelineEditorPanel::SetDuration(float s) { m_impl->state.duration = s; }
void TimelineEditorPanel::SetZoom(float pps)   { m_impl->state.zoomLevel = pps; }

void TimelineEditorPanel::OnPlayheadMoved(std::function<void(float)> cb) {
    m_impl->onPlayheadMoved = std::move(cb);
}

void TimelineEditorPanel::OnKeyframeSelected(
    std::function<void(uint32_t, float)> cb) {
    m_impl->onKeyframeSelected = std::move(cb);
}

} // namespace Editor
